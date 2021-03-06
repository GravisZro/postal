#!/bin/sh

################################################################################
#                                                                              #
#                               Shell functions                                #
#                                                                              #
################################################################################

# Function to print to stdout and log
# @param[in] $@ message
announce () {
  echo "${@}" | tee -a ${log}
}

# Function to record error to log
# @param[in] $1 message
log_error () {
  announce "ERROR: ${1}"
}

# Function to record warning to log
# @param[in] $1 message
log_warning () {
  announce "WARNING: ${1}"
}

# Function to convert a string to lowercase
# @param[in] $1 string
to_lower () {
  echo $1 | tr '[:upper:]' '[:lower:]'
}

# Function concatonate strings into a single string
# @param[in] $@ input strings
concat () {
  until [ "x${1}" = "x" ]
  do
    echo -n "${1}"
    shift
  done
}

# Function returns the length of a string
# @param[in] $1 input string
strlen () {
  echo ${#1}
}


# Function to extract value from argument
# @param[in] $1 input string e.g. --flagname=value
arg_value () {
  echo $1 | sed -e "s/--[a-z\-]*=\(.*\)/\1/"
}

# Function to replace a value in an argument
# @param[in] $1 input string e.g. "--flag1=value1 --flag2=value2"
# @param[in] $2 argment name e.g. "--flag1"
# @param[in] $3 replacement string e.g. "newvalue"
replace_value() {
  echo $1 | sed -e "s/\($2=\)\S\+/\1$3/"
}

# Function that faults if a directory doesn't exist
# @param[in] $1 program name
# @param[in] $2 program directory
assert_dir () {
  if [ ! -e "${2}" ]
  then
    log_error "Directory for ${1} is missing! Expected: ${2}"
    exit 1
  fi
}

# Function to check for relative directory and makes it absolute
# @param[in] $1  The directory to make absolute if necessary.
absolutedir()
{
  case ${1} in
    /*)
      echo "${1}"
    ;;

    *)
      echo "${PWD}/${1}"
    ;;
  esac
}

# Function to detect if a program is usable
# @param[in] $1 program
# @return the result of the underlying call or 1 if no utility is found
detect () {
  eval "${1} --version >> /dev/null 2>&1"
  if [ $? -ne 0 ]
  then
    echo "false"
  fi
  echo "true"
}

find_bin ()  {
  files=""
  paths="$PATH:"
  until [ "x$paths" = "x" ]
  do
    curpath=$(echo $paths | sed -e "s/\([^:]*\):.*/\1/")
    paths=$(echo $paths | sed -e "s/[^:]*:\(.*\)/\1/")

    result=$(echo $(ls $curpath 2>/dev/null) | sed -e "s/.*\($1\).*/\1/")
    count=$(echo $result | wc -w) # ensure we actually found it
    if [ $count = 1 ] && [ "x$result" != "x" ]
    then
      echo $result
      paths=""
    fi
  done
}

################################################################################
#                                                                              #
#                               Initialize setup                               #
#                                                                              #
################################################################################

# Determine the number of processes to use for building
makejobs=$(nproc --all)
if [ $? -ne 0 ]
then
  makejobs=$(sysctl hw.ncpu | cut -f2 -d' ')
  if [ $? -ne 0 ]
  then
    makejobs="1"
  fi
fi

has_make=$(detect "make")
has_gmake=$(detect "gmake")
if ${has_make}
then
  make_tool="make"
elif ${has_gmake}
then
  make_tool="gmake"
else
  log_error "No known \"make\" program could be found!"
  exit 1
fi

case $(uname -s) in
  Darwin)
    current_platform="osx";
  ;;
  *)
    current_platform="posix";
  ;;
esac

# seek compiler for current system
if $(detect "g++")
then
  eval "platform_${current_platform}=true"
  export CXX="g++"
elif $(detect "clang")
then
  eval "platform_${current_platform}=true"
  export CXX="clang"
fi


# a small helper to keep it clean
detect_platform () {
  eval "platform_$1=$(find_bin $2)"
}

os_name=$(uname -s | sed -e "s/\(.\)/\l\1/")
availible_platforms=""

detect_platform "posix" "x86_64-\S*${os_name}\S*-g++"
if [ -z "${platform_posix}" ]
then
  detect_platform "posix" "i[0-9]86-\S*${os_name}\S*-g++"
  if [ ! -z "${platform_posix}" ]
  then
    availible_platforms="$availible_platforms|POSIX"
  fi
else
  availible_platforms="$availible_platforms|POSIX"
fi

# seek special compilers
detect_platform "dos" "i.86-\S\+djgpp-g++"
detect_platform "win32" "i.86-\S\+-mingw32-g++"
detect_platform "win64" "x86_64-\S\+-mingw32-g++"
detect_platform "dreamcast" "dreamcast-sh-g++"
detect_platform "saturn" "saturn-sh-g++"


if [ ! -z ${platform_dos} ]
then
  availible_platforms="$availible_platforms|DOS"
fi
if [ ! -z ${platform_osx} ]
then
  availible_platforms="$availible_platforms|OSX"
fi
if [ ! -z ${platform_win32} ] || [ ! -z ${platform_win64} ]
then
  if [ ! -z ${platform_win32} ] && [ -z ${platform_win64} ]
  then
    platform_bits="32"
  elif [ -z ${platform_win32} ] && [ ! -z ${platform_win64} ]
  then
    platform_bits="64"
  fi
  availible_platforms="$availible_platforms|Windows"
fi
if [ ! -z ${platform_dreamcast} ]
then
  availible_platforms="$availible_platforms|Dreamcast"
fi
if [ ! -z ${platform_saturn} ]
then
  availible_platforms="$availible_platforms|Saturn"
fi
if [ -z $availible_platforms ]
then
  log_error "No C++ compilers could be found!"
  exit 1
fi


################################################################################
#                                                                              #
#                               Parse arguments                                #
#                                                                              #
################################################################################
# Defaults

cpp_standard="-std=c++11"
c_standard="-std=c11"
platform=$(to_lower $(uname -s))
arch=$(uname -m)
verbose=false
noeditor=true
nomultiplayer=true
makefile="Makefile.gravis"
make_target=
build_name="postal"
backend="sdl2"
ldflags="-lSDL2"
backend_sources="BLUE/Bdebug.cpp \
                 BLUE/Bjoy.cpp \
                 BLUE/Bkey.cpp \
                 BLUE/Bmain.cpp \
                 BLUE/Bmouse.cpp \
                 BLUE/Btime.cpp \
                 BLUE/Bdisp.cpp \
                 BLUE/Bsound.cpp \
                 CYAN/uDialog.cpp \
                 CYAN/uColors.cpp \
                 CYAN/uPath.cpp"
case $arch in
  x86_64)
  platform_bits=64
  ;;
  *)
    platform_bits=32
  ;;
esac

basedir=$(absolutedir $(dirname $0))
builddir=$(absolutedir "${basedir}/builds")
installdir="/usr/local"

# strip leading '|' from availible_platforms
availible_platforms=$(expr substr "$availible_platforms" 2 999)

defines=""

nextline="\n"
len=$(strlen "Usage: ${0}")
until [ $len -eq 0 ]
do
  len=$((len-1))
  nextline="$nextline "
done

usage=$(concat \
      "\nUsage: ${0} [--target=1997|Plus|Super|2015]" \
      "$nextline [--locale=US|UK|Germany|France|Japan]" \
      "$nextline [--platform=$availible_platforms]" \
      "$nextline [--platform-bits=32|64]" \
      "$nextline [--demo-client | --multiplayer-client]" \
      "$nextline [--with-steam]" \
      "$nextline [--with-joystick | --with-twinstick]" \
      "$nextline [--disable-multiplayer | --with-multiplayer | --without-multiplayer]" \
      "$nextline [--disable-editor | --with-editor | --without-editor]" \
      "$nextline [--install]" \
      "$nextline [--uninstall]" \
      "$nextline [--builddir=<dir>]" \
      "$nextline [--installdir=<dir>]" \
      "$nextline [--makejobs=<number>]" \
      "$nextline [--verbose]" \
      "$nextline [--release | --debug | --clean]" \
      "$nextline [--tracelog=<filename>]" \
      "$nextline [--sakmanip]" \
      "$nextline [--help | -h]" \
      )
help=$(concat \
      "\n Options are case insensitive :)" \
      "\n TODO: create \$help string" \
      )

until [ "x$1" = "x" ]
do
  option=$(to_lower $1)
  case ${option} in
    --verbose)
      verbose=true;
    ;;

    --target=*)
      case $(arg_value ${option}) in
        1997)
          defines="$defines TARGET=POSTAL_1997"
        ;;
        2015)
          defines="$defines TARGET=POSTAL_2015"
        ;;
        plus) # Plus
          defines="$defines TARGET=POSTAL_PLUS"
        ;;
        super) # Super
          defines="$defines TARGET=SUPER_POSTAL"
        ;;
        *)
          echo "error: ${option}"
          echo "Unrecognized target name.  Valid values are: 1997, Plus, Super and 2015."
          exit 1
        ;;
      esac
    ;;

    --locale=*)
      case $(arg_value ${option}) in
        us|usa) # US | USA
          defines="$defines LOCALE=US"
        ;;
        uk) # UK
          defines="$defines LOCALE=UK"
        ;;
        gr|germany|german)
          defines="$defines LOCALE=Germany"
        ;;
        fr|france|french)
          defines="$defines LOCALE=France"
        ;;
        jp|japan)
          defines="$defines LOCALE=Japan"
        ;;
        *)
          echo "error: ${option}"
          echo "Unrecognized locale name.  Valid values are: US, UK, Germany, France and Japan."
          exit 1
        ;;
      esac
    ;;

    --cpp-compiler=*)
      cpp_compiler=$(arg_value ${option})
    ;;

    --platform=*)
      platform=$(arg_value ${option})
      case ${platform} in
        *32)
          platform_bits="32"
          platform=$(echo $platform | sed -e "s/\(\S\+\)32/\1/")
        ;;
        *64)
          platform_bits="64"
          platform=$(echo $platform | sed -e "s/\(\S\+\)64/\1/")
        ;;
      esac
    ;;

    --platform-bits=*)
      platform_bits=$(arg_value ${option})
      if [ "$platform_bits" != "32" ] &&  [ "$platform_bits" != "64" ]
      then
        echo "error: ${option}"
        echo "You may only select 32-bits or 64-bits for a platform."
        exit 1
      fi
    ;;

    --demo-client)
      defines="$defines DEMO"
    ;;

    --multiplayer-client)
      defines="$defines SPAWN"
    ;;

    --with-steam)
      defines="$defines STEAM_CONNECTED"
      cflags="$cflags -Isteamworks/sdk/public"
    ;;

    --with-joystick)
      defines="$defines ALLOW_JOYSTICK"
    ;;

    --with-twinstick)
      defines="$defines ALLOW_TWINSTICK"
    ;;

    --disable-multiplayer)
      defines="$defines MULTIPLAYER_DISABLED"
    ;;

    --with-multiplayer)
      nomultiplayer=false;
    ;;

    --wihout-multiplayer)
      nomultiplayer=true;
    ;;

    --disable-editor)
      defines="$defines EDITOR_DISABLED"
    ;;

    --with-editor)
      noeditor=false;
    ;;

    --without-editor)
      noeditor=true;
    ;;

    --install)
      install=true
    ;;

    --uninstall)
      uninstall=true
    ;;

    --installdir=*)
      installdir=$(absolutedir $(arg_value ${option}))
    ;;

    --builddir=*)
      builddir=$(absolutedir $(arg_value ${option}))
    ;;


    --clean)
      make_target="clean"
    ;;

    --release)
      make_target="release"
    ;;

    --debug)
      make_target="debug"
    ;;

    --tracelog=*)
      defines="$defines RSP_DEBUG_OUT_FILE RSP_TRACE_LOG_NAME=$(arg_value ${option})"
    ;;

    --makejobs=*)
      makejobs=$(arg_value ${option})
    ;;

    --sakmanip)
      echo -n "Building SAK Manipulation Tool... "
      g++ -I. newpix/sakarchive.cpp newpix/filestream.cpp newpix/sakmanip.cpp -o sakmanip
      echo "Done!"
      exit 0
    ;;

    -h|--help)
      echo "${usage}"
      echo "${help}"
      exit 0
    ;;

    ?*)
      option=$(echo ${option} | sed -e "s/\(--[a-z]*\)=.*/\1/")
      echo "\nUnrecognized option: ${option}"
      echo "${usage}"
      exit 1
    ;;

    *)
    ;;
  esac
  shift
done

if ${nomultiplayer}
then
  defines="$defines MULTIPLAYER_REMOVED"
fi

if ${noeditor}
then
  defines="$defines EDITOR_REMOVED"
fi


force_32bit() {
  if [ "$platform_bits" = "64" ]
  then
    echo "Forcing 32-bit mode for target: $1"
  fi
  platform_bits=32
}

if [ -z "$cpp_compiler" ]
then
  case ${platform} in
  linux|osx|macos|mac|darwin|freebsd|openbsd|netbsd|gnu/kfreebsd|dragonfly|haiku|cygwin_nt-*|posix) # POSIX
      cpp_compiler=$platform_posix
      platform="POSIX"

      if [ "$os_name" = "linux" ] && [ "$platform_bits" = "32" ] # linux 32-bit
      then
        archflags="-m32"
        ldflags="-L./sys/lib/linux-x86 -lSDL2"
        steamlib="steamworks/sdk/redistributable_bin/linux32/libsteam_api.so"
      elif [ "$os_name" = "darwin" ]
      then
        archflags="-mmacosx-version-min=10.5"
        ldflags="-framework CoreFoundation -framework Cocoa"
        if [ "$platform_bits" = "32" ] # OSX 32-bit
        then
          archflags="-arch i386 $archflags"
          ldflags="-L./sys/lib/macosx -lSDL2"
          steamlib="steamworks/sdk/redistributable_bin/osx32/libsteam_api.dylib"
        fi
      elif [ "$platform_bits" = "32" ] # unknown 32-bit
      then
        archflags="-m32"
      fi
      build_name="${build_name}.$(to_lower $platform)${platform_bits}"
    ;;

    gnu) # GNU Hurd
      cpp_compiler=$platform_posix
      force_32bit "GNU Hurd"
      platform="POSIX"
    ;;

    windows) # Windows
      case $platform_bits in
        32)
          cpp_compiler=$platform_win32
          archflags="-m32"
          ldflags="-L./sys/lib/win32 -lmingw32 -lSDL2main -lSDL2"
          steamlib="steamworks/sdk/redistributable_bin/win32/libsteam_api.lib" # just guessing
        ;;
        64)
          cpp_compiler=$platform_win64
          archflags="-m64"
        ;;
      esac
      build_name="${build_name}w${platform_bits}.exe"
      platform="Windows"
    ;;

    *dos) # *DOS
      cpp_standard="-std=gnu++11"
      c_standard="-std=gnu11"
      cpp_compiler=$platform_dos
      force_32bit "DOS"
      build_name="${build_name}.exe"
      platform="DOS"
      backend="dos"
      ldflags=""
      archflags="-m32 -march=i586 -mtune=i586"
      backend_sources="${backend_sources} BLUE/platform.cpp"
    ;;

    dreamcast) # Sega Dreamcast
      cpp_standard="-std=gnu++11"
      c_standard="-std=gnu11"
      cpp_compiler=$platform_dreamcast
      force_32bit "Dreamcast"
      build_name="${build_name}SH4.elf"
      platform="Dreamcast"
      cflags="$cflags -ffunction-sections -fdata-sections"
      backend="dreamcast"
      ldflags=""
    ;;

    saturn) # Sega Saturn
      cpp_compiler=$platform_saturn
      force_32bit "Saturn"
      build_name="${build_name}SH2.elf"
      platform="Saturn"
      backend="saturn"
      ldflags=""
    ;;

    *)
      echo "Unsupported platform.  Try one of these: $availible_platforms"
      exit 1
    ;;
  esac
fi

echo $defines | grep "STEAM_CONNECTED" > /dev/null 2>&1
if [ $? = 0 ]
then
  ldflags="$ldflags $steamlib"
fi

# remove excess whitespace
backend_sources=$(echo $backend_sources | sed -e "s/\s\+/ /g")

echo "Platform: $platform"
echo "Bits: $platform_bits"
echo "Compiler: $cpp_compiler"
echo "Platform flags: $archflags"
echo "Linker flags: $ldflags"
echo "Options: $defines"
echo "Executable: $build_name"

if [ -z $make_target ]
then
  make_target=$build_name
fi

echo "executing: make -f $makefile $make_target -e VERBOSE=\"$verbose\" -e CC=\"$cpp_compiler\" FLAGS=\"$cflags\" LDFLAGS=\"$ldflags\" -e DEFINES=\"$defines\" -e POSTAL_ARCHFLAGS=\"$archflags\" -e POSTAL_BINARY=\"$build_name\" -e BACKEND=\"$backend\" -e CPP_STANDARD=\"$cpp_standard\" -e C_STANDARD=\"$c_standard\" -e BUILD_PATH=\"bin/$(to_lower $platform)${platform_bits}\" -e BACKEND_SOURCES=\"$backend_sources\""

make -j$makejobs -f $makefile $make_target -e VERBOSE="$verbose" -e CC="$cpp_compiler" FLAGS="$cflags" LDFLAGS="$ldflags" -e DEFINES="$defines" -e POSTAL_ARCHFLAGS="$archflags" -e POSTAL_BINARY="$build_name" -e BACKEND="$backend" -e CPP_STANDARD="$cpp_standard" -e C_STANDARD="$c_standard" -e BUILD_PATH="bin/$(to_lower $platform)${platform_bits}" -e BACKEND_SOURCES="$backend_sources"

exit 0


if ${uninstall}
then
  echo -n "\nUninstalling..."
  sudo rm -Rf ${installdir}/${platform}
  echo "\n======= [ Uninstall complete! ] ======="
  if ! ${install}
  then
    exit 0
  fi
fi

if ! ${install}
then
  echo -n "Build Postal? "
  read doinstall
  case $(to_lower ${doinstall}) in
    y|yes)
    ;;
    *)
      echo "\nAborting installation\n"
      eval "sh ${0} --help"
      echo ""
      exit 1
    ;;
  esac
fi




################################################################################
#                                                                              #
#                                Build Process                                 #
#                                                                              #
################################################################################


#PANDORA
  CFLAGS += -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -ftree-vectorize -ffast-math -DPANDORA
#ODROID
  CFLAGS += -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard -ftree-vectorize -ffast-math -DODROID

