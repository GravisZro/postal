////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2016 RWS Inc, All Rights Reserved
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of version 2 of the GNU General Public License as published by
// the Free Software Foundation
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
// ResMgr.h
//
// Created on: 12/09/96 BRH
// Implemented:12/09/96 BRH
//
// Resource manager class which allows multiple access to shared
// resources to save load time and memory.
//
//	The resource manager can load resources either from standard
//	disk files, or from a SAK file which is a single file that
// contains many resources.  The SAK files are used to encapsulate
// data and make seeking for resources faster than seeking for
// individual disk files especially on a CD.  If a SAK file is
// not loaded, then the resource names given are assumed to be
// disk pathnames and the resources are loaded from these files.  
// When a SAK file is open, then the resource names are assumed to
// refer to resources contained in that SAK file.  If you ask 
// for a resource name that is not in the SAK file, it will result
// in an error - file not found in SAK file.  This was done to 
// eliminate confusion in testing, we didn't want the load to try
// the disk file because it would be confusing to the user if they
// thought all of the resources they were loading were contained in
// the SAK file they loaded.  If you close the SAK file, then the
// resource names are assumed to refer to individual disk files again.
//
// The RResMgr class is completely self contained.  It includes two
// utility functions that are used to create SAK files.  Statistics()
// or CreateSakBatch() will create a text script file that can be used 
// to create SAK files.  The file it generates has a list of the 
// resources in the order they were accessed along with 
// their resource type.  It then also includes a comment block
// on the total number of accessed for each resource.  The 
// CreateSak(scriptfile, sakfile) takes the name of the text
// script file generated by Statistics() and the name of the SAK file
// you wish to create.  It will load each of the resouces in the
// script file and save them into the SAK file with a 
// resource name/offset directory table at the beginning of the SAK
// file.  When you open a SAK file, this directory is read in and
// cached and the SAK file remains open.  Then when you Get() a 
// resource, it gets the offset in the file using the directory
// and then seeks to the correct position in the open SAK file
// to retrieve your resource.
//
// You could make your own SAK files by typing in a script file
// and then calling CreateSak(), but it is easier to run your
// code loading resources from their individual files and then at 
// some point in your code after you have loaded all of your resources, 
// call CreateSakBatch() to make the batch file for you.  If you
// want to create a SAK file, you can immediately afterward call
// CreateSak() giving the name of the SAK batch file you just
// created and it will create a SAK file for you.  Then you can 
// modify your code to remove the SAK creation and add a OpenSak()
// at the beginning of your code to enable it to load your resources
// from the SAK file instead of individual resource files.  Also
//	add a CloseSak() after you are done loading resources.
//
// When you load a resource for the first time, it is loaded, either
// from the resource file or the SAK file (as described above) and
// then if the resource is requested again, it is already in memory
// and is just given out.  When you are done with a resource, you
// should call Release, just as you would normally call Close.  
// When you call Release, the resource is still in memory for the
// next time it is requested.  If you wish to free memory for 
// resources that are no longer being used, you can call Purge()
// which will free all resources with a zero reference count.  
// Resources that still haven't been Released() will not be purged.
// If a resource was Purged and is then requested again, it will 
// be loaded from the resource file or SAK.  
//
// History:
//		12/09/96 BRH	Started this class using STL containers to
//							keep track of resources that have been loaded.
//
//		12/17/96 BRH	Added additional resource types for Get and 
//							Release.  Also added statistics tracking and
//							reporting.  The statistics report can be used
//							to generate the SAK files.
//
//		12/18/96 BRH	Added CreateSak() function to read in the 
//							text script file generated by Statistics()
//							and create a SAK file.  Also added OpenSak()
//							function to read in the SAK file, create the
//							internal directory and have an open RFile 
//							available from which the resources will be read.
//							Once a SAK file has been opened, it is assumed
//							that all resource names refer to the SAK file, 
//							so accessing resources that are not in the SAK
//							file will be an error, it will not attempt to
//							read the resource from a disk file.  CloseSak()
//							will close the SAK file and resource names will
//							go back to looking at disk files.
//
//		12/19/96 BRH	Added RFont as one of the supported resources.
//
//		01/06/97 BRH	Added generic void type of resource which loads
//							a buffer of bytes which can then be plugged into
//							an RFile as a memory file as one example.
//
//		01/29/97	JMI	Added RSample as one of the supported resources.
//
//		02/03/97 BRH	Added a function FromSystempath() that takes
//							a resource name and converts it into a system
//							pathname when loading individual files.  It is
//							similar to the FromSak() function.  The Get
//							functions now use this FromSystempath funciton
//							to prepend the base path to the resource name
//							in order to load files.  Also added more
//							checking to SetBasePath function to make sure
//							its in the correct system path format.
//
//		02/03/97 BRH	Made FromSak() function public so that it could
//							be used to directly access the SAK file without
//							involving the resource manager.
//
//		02/03/97 BRH	Added RAttributeMap as a supported type.
//
//		02/04/97	JMI	Added rspGetResource(), rspReleaseResource(),
//							and helper templates (ResMgr_CreateResource,
//							ResMgr_DestroyResource, ResMgr_LoadResource,
//							and ResMgr_SaveResource).  Also added member
//							to a member version of Get() in RResMgr that
//							supports just the kind of functionality that
//							the template functions need.
//							To make this generic resource interface 
//							possible, CResourceBlock had to be modified to 
//							include pointers to Create, Destroy, Load, and
//							Save (aslo, m_usType was no longer necessary).
//							Dropped support for particular types and got rid
//							of the void resource.
//
//		02/10/97	JMI	rspReleaseResource() now takes a ptr to a ptr.
//							Got rid of ResMgr_Load() and ResMgr_Save() b/c
//							we have now introduced this same concept im-
//							plemented at a lower level (RFile) called
//							rspAnyLoad() and rspAnySave().
//
//		03/25/97 BRH	Added NormalizeResName() function to make sure
//							any resource name is converted to an rsp path
//							using / instead of \ and using all lower case
//							so that the matching of names works.
//
//		04/25/97	JMI	FromSak() now fails if a resource is not in the
//							SAK directory map.
//
//		05/08/97	JMI	Added conditions for compiler versions' STL
//							differences.
//
//		05/30/97	JMI	Added RESMGR_VERBOSE.  Which, when defined, 
//							causes the resmgr to output TRACEs when a file
//							is not found.  Although we all love this feature
//							for debugging purposes, it gets bad when we're
//							loading a file only if it exists.
//
//		06/11/97 BRH	Added rspReleaseAndPurgeResource template which 
//							will purge a given resource as long as its 
//							reference count is zero, otherwise it acts 
//							as a rspReleaseResource.
//
//		06/29/97 MJR	Minor changes for mac compatibility.
//
//		07/09/97	JMI	Made pszResName parm to rspGetResource() a const.
//
//		07/17/97	JMI	Added rspGetResourceInstance(), GetInstance(),
//							and rspReleaseResourceInstance().
//
//		08/12/97	JMI	Added GetBasePath() to return the base path.
//
//		08/28/97 BRH	Added a flag and a trace to show uncached loads
//							if the flag is set it will print a trace of 
//							any resources that have to loaded from a disk.
//							This can be used in a game to make sure that
//							things aren't loading during gameplay if they
//							were meant to be preloaded.
//
//		09/03/97	JMI	Now rspGetResourceInstance() allocates the 
//							create, destroy, and load funcs on the stack 
//							since they're just needed temporarily.  I think
//							that this was probably a memory leak before.
//
//////////////////////////////////////////////////////////////////////
#ifndef RESMGR_H
#define RESMGR_H

#include <cctype>

#include <BLUE/System.h>

#include <CYAN/cyan.h>
#include <ORANGE/File/file.h>
#include <ORANGE/RString/rstring.h>

#if _MSC_VER >= 1020 || __MWERKS__ >= 0x1100 || __GNUC__
# include <map>
# include <vector>
# include <set>
# include <functional>
# include <algorithm>
# include <memory>
#else
# include <map.h>
# include <vector.h>
# include <set.h>
#endif

#define SAK_COOKIE 0x204B4153		// Looks like "SAK " in the file
#define SAK_CURRENT_VERSION 1		// Current version of SAK file format

///////////////////////////////////////////////////////////////////////////////
//
// Function objects
//
// These strange-looking structs with an overloaded operator() are really just
// fancy function pointers.  They are often referred to as "function objects".
//
// There are several reasons for using function objects instead of function
// pointers.  For instance, C++ won't instantiate a template based on an
// argument being passed to a function.  You could get around this by
// instantiating the object ahead of time, but function objects take care of
// it for you.  Another problem is that certain techqniues, such as trying to
// get the compiler to instantiate a template based on a cast to the desired
// type, doesn't work on all compilers (that particular technique works under
// Visual C++ but not under Metrowerks).
//
// The resource manager was originally implimented using function pointers
// and relied on the compiler being able to instantiate function templates as
// a result of a cast to the desired type.  This didn't work under CodeWarrior,
// so we had to switch to function objects.
//
// The only remaining wrinkle was that once we had all these different types
// of function pointers, we needed to pass them all to the same function in
// the resource manager, and this function could NOT be templated.  The
// solution was to derive the templated function objects from a common,
// non-templated base class.  We could then pass any specific type to a
// function that accepted the base type.
//
// The overloaded operator() had to be virtual for this to work properly, and
// this is the only thing that I would still like to improve upon.  Virtual
// functions can't be inlined because they are evaluated at runtime.  However,
// this is a trivial amount of overhead considering everything else the
// resource manager is doing.
//
///////////////////////////////////////////////////////////////////////////////

// Base class and templated derived class of function object used to
// create a specific type of resource.  The function simply does a "new".
// We had to use a handle to the object instead of just returning a pointer
// to it because you can't overload functions based soley on return type.
struct GenericCreateResFunc
	{
  virtual ~GenericCreateResFunc(void) { }
	virtual int16_t operator()(void** ppT)
		{
		*ppT = 0;
      return FAILURE;	// generic version should never be called!
		}
	};

template<class T>
struct CreateResFunc : GenericCreateResFunc
	{
	int16_t operator()(void** ppT)
		{
		*ppT = (void*)new T;
		return *ppT ? 0 : -1;
		}
	};


// Base class and templated derived class of function object used to
// destroy a specific type of resource.  The function simply does a "delete". 
struct GenericDestroyResFunc
	{
  virtual ~GenericDestroyResFunc(void) { }
	virtual void operator()(void* /*pT*/)
		{  }
	};

template<class T>
struct DestroyResFunc : GenericDestroyResFunc
	{
	void operator()(void* pT)
		{ delete (T*)pT; }
	};


// Base class and templated derived class of function object used to
// load a specific type of resource.  The function uses rspAnyLoad().
struct GenericLoadResFunc
	{
  virtual ~GenericLoadResFunc(void) { }
	virtual int16_t operator()(void* /*pT*/, RFile* /*pfile*/)
      { return FAILURE; }	// generic version should never be called!
	};

template<class T>
struct LoadResFunc : GenericLoadResFunc
	{
	int16_t operator()(void* pT, RFile* pfile)
		{ return rspAnyLoad((T*)pT, pfile); }
	};


///////////////////////////////////////////////////////////////////////////////
//
// Resource manager block
//
///////////////////////////////////////////////////////////////////////////////
class CResourceBlock
{
	public:

		int16_t  m_sRefCount;
		int16_t  m_sAccessCount;
		void*  m_vpRes;
		RString m_strFilename;
		GenericDestroyResFunc* m_pfnDestroy;

		CResourceBlock()
			{
			m_sRefCount = 0;
			m_sAccessCount = 0;
			m_vpRes = nullptr;
			m_pfnDestroy = 0;
         }

		~CResourceBlock()
			{
			FreeResource();

			// Delete the function object
			delete m_pfnDestroy;
			m_pfnDestroy = 0;
         }

		void FreeResource(void)
			{
			if (m_vpRes)
				{
				// Must have the destroy function.
				ASSERT(m_pfnDestroy);

				// Destroy resource.
				(*m_pfnDestroy)(m_vpRes);
				m_vpRes = 0;
				}
			}

};

#if _MSC_VER >= 1020 || __MWERKS__ >= 0x1100
	#if __MWERKS__ >= 0x1100
		ITERATOR_TRAIT(const RString);
	#endif
  typedef std::map<RString, CResourceBlock, std::less<RString>, std::allocator<CResourceBlock> > resclassMap;
  typedef std::map<void*, RString, std::less<void*>, std::allocator<RString> > ptrLookupMap;
  typedef std::vector<RString, std::allocator<RString> > accessVector;
  typedef std::set<RString, std::less<RString>, std::allocator<RString> > dupSet;
  typedef std::vector<uint16_t, std::allocator<uint16_t> > typeVector;
  typedef std::map<RString, int32_t, std::less<RString>, std::allocator<int32_t> > dirMap;
  typedef std::set<int32_t, std::less<int32_t>, std::allocator<int32_t> > dirOffsets;
#else
  typedef std::map<RString, CResourceBlock, std::less<RString> > resclassMap;
  typedef std::map<void*, RString, std::less<void*> > ptrLookupMap;
  typedef std::vector<RString > accessVector;
  typedef std::set<RString, std::less<RString> > dupSet;
  typedef std::vector<uint16_t > typeVector;
  typedef std::map<RString, int32_t, std::less<RString> > dirMap;
  typedef std::set<int32_t, std::less<int32_t> > dirOffsets;
#endif


///////////////////////////////////////////////////////////////////////////////
//
// Resource Manager class
//
///////////////////////////////////////////////////////////////////////////////
class RResMgr
{
	////////////////////////////////////////////////////////////////////////////
	// Typedefs.
	////////////////////////////////////////////////////////////////////////////
	public:
		
	public:
		
		// Constructor
		RResMgr();

		// Destructor
		~RResMgr();

		// void load
		int16_t Get(												// Returns 0 on success.
			RString strFilename,								// In:  Resource name
			void** hRes,										// Out: Pointer to resource returned here
			RFile::Endian	endian,							// In:  Endian nature of resource file
			GenericCreateResFunc* pfnCreate,				// In:  Pointer to "create" function object
			GenericDestroyResFunc* pfnDestroy,			// In:  Pointer to "destroy" function object
			GenericLoadResFunc* pfnLoad);					// In:  Pointer to "load" function object

		int16_t GetInstance(									// Returns 0 on success.
			RString strFilename,								// In:  Resource name
			void** hRes,										// Out: Pointer to resource returned here
			RFile::Endian	endian,							// In:  Endian nature of resource file
			GenericCreateResFunc* pfnCreate,				// In:  Pointer to "create" function object
			GenericDestroyResFunc* pfnDestroy,			// In:  Pointer to "destroy" function object
			GenericLoadResFunc* pfnLoad);					// In:  Pointer to "load" function object

		// void release
		void Release(void* pVoid);

		// Purge - deallocate a single resource with a zero reference count
		bool ReleaseAndPurge(void* pVoid);

		// Purge - deallocate all resources with a zero reference count
		void Purge(void);

		// Function to turn on or off tracing of non-cached loads
		void TraceUncachedLoads(bool bShow)
		{
			m_bTraceUncachedLoads = bShow;
		}

		// Statistics for analysis purposes to see how well the
		// resources are being utilized and produce a batch file
		// that can be used to make a SAK file.  This function takes
		// a filename and produces a text file giving the list
		// of files that were accessed and their statistics.
		int16_t Statistics(RString strStatFile);

		// Just a more obvious function name for creating
		// the batch files.
		int16_t CreateSakBatch(RString strBatchFile)
		{
			return Statistics(strBatchFile);
		}

		// Read in one of the script files created by Statistics()
		// and create a SAK file of the given name.  
		int16_t CreateSak(RString strScriptFile, RString strSakFile);

		// Open a SAK file and until it is closed, assume that
		//	all resource names refer to resources in this SAK file.
		//	If a resource name is not in the SAK file, then it cannot
		// be loaded.  It does not attempt to load the resource from
		// its individual disk file.
		int16_t OpenSak(RString strSakFile);

		// Open an Alternate SAK file
		// with an optionnal script file to overload name in Alternate SAK
		// (used for XMas runtime patch)
		int16_t OpenSakAlt(RString strSakFile, RString strScriptFile = false);

		// This function closes the Alt SAK file and all resource names
		void CloseSakAlt()
		  {
			  if (m_rfSakAlt.IsOpen())
			    {
					m_rfSakAlt.Close();
					m_SakAltDirectory.erase(m_SakAltDirectory.begin(), m_SakAltDirectory.end());
			    }
		  }

		// This function closes the SAK file and all resource names
		// are assumed to refer to individual disk files.
		void CloseSak()
			{ if (m_rfSak.IsOpen())
				{
					m_rfSak.Close();
					m_SakDirectory.erase(m_SakDirectory.begin(), m_SakDirectory.end());
					CloseSakAlt();
				}
			}

		// This function sets a base pathname that will be prepended to
		// the resource name when loading resources from their individaul
		// files.
		void SetBasePath(RString strBase);

		// This function returns the base path.
		char* GetBasePath(void)
			{
			return (char*)m_strBasepath;
			}

		// Helper function to position m_rfSak at correct position
		// for the file you are trying to get
		RFile* FromSak(RString strResourceName)
		{
			RFile* prf = nullptr;
			if (m_rfSakAlt.IsOpen()) 
			  {
				int32_t	lResSeekPos	= m_SakAltDirectory[strResourceName];
				if (lResSeekPos > 0)
					{
					if (m_rfSakAlt.Seek(lResSeekPos, SEEK_SET) == SUCCESS)
						{
						prf = &m_rfSakAlt;
						return prf;
						}
					else
						{
						TRACE("RResMgr::FromSak - m_rfSakAlt.Seek(%i, SEEK_SET) failed.\n", 
							lResSeekPos);
						}
					}
			  }
			int32_t	lResSeekPos	= m_SakDirectory[strResourceName];
			if (lResSeekPos > 0)
				{
				if (m_rfSak.Seek(lResSeekPos, SEEK_SET) == SUCCESS)
					{
					prf = &m_rfSak;
					}
				else
					{
					TRACE("RResMgr::FromSak - m_rfSak.Seek(%i, SEEK_SET) failed.\n", 
						lResSeekPos);
					}
            }
			else
            TRACE("RResMgr::FromSak - Resource %s is not in this SAK file\n",
				      (char*) strResourceName);
//				      (char*) strResourceName.c_str());

			return prf;
		}


	private:
	
		// Convert the resource name to an rsp resource name with / slashes, 
		// and make sure that it is all lower case to avoid compare problems.

		void NormalizeResName(RString* pstrResourceName)
		{
			int32_t i;
			for (i = 0; i < pstrResourceName->GetLen(); i++)
			{
				if (pstrResourceName->GetAt(i) == '\\')
					pstrResourceName->SetAt(i, '/');
				pstrResourceName->SetAt(i, tolower(pstrResourceName->GetAt(i)));
			}
		}

		// This flag will print trace messages of non-cached loads.  You can
		// turn it on after a point in the app where you expect everything 
		// should be loaded into the cache, then it will show you what
		// resources were loaded from disk after that point.
		bool m_bTraceUncachedLoads;

		// m_map is a map of filenames to CResourceBlocks for fast
		// access using the resource filename for lookup
		resclassMap m_map;

		// m_ptrMap is a map of allocated resource pointers to filenames
		// so that you can lookup the original resource filename once
		// you have a pointer to that resource.  It is used when freeing
		// the resource, it looks up the resource name using the pointer
		// and then looks up the CResouceBlock using the resource name.
		ptrLookupMap m_ptrMap;

		// m_duplicateSet is used in Statistics() to eliminate 
		// duplicates from the m_accessList so that it prints only
		// the unique files in the order that they were accessed.  
		// The access order is preserved to try to minimize seeking 
		// in the SAK file.
		dupSet m_duplicateSet;

		// m_accessList keeps track of the access order of the resources
		// and just adds a resource name each time that file is loaded.  
		// If Purge was called and then the file was reloaded, it will be
		// in this list twice, but the duplicates will be eliminated
		// as the SAK file list is being created.
		accessVector m_accessList;

		// These are used for creating a SAK file

		// m_LoadList is the list of filenames to be loaded in
		// the order that they appear in the SAK script file.  
		accessVector m_LoadList;

		// m_TypeList parallels the m_LoadList so we know what 
		// type of resource needs to be created in order to load
		// and save this type of resource.
//		typeVector	 m_TypeList;

		// m_DirectoryMap is a mapping of resource names to offsets
		// within the SAK file.  When reading from a SAK file, the
		// resource name will give the offset to seek to in order
		// to load the given resource.
		dirMap		 m_DirectoryMap;

		// m_SakDirectory is a mapping of resource names to offsets
		// within the SAK file.  This is separate from the m_DirectoryMap
		// just in case someone calls CreateSak while a sak file is already
		// loaded.  This is the map used for the open SAK file.
		dirMap		m_SakDirectory;

		// With the addition of the CResVoid class to support generic
		// data blocks with unknown length, it was necessary to add this
		// container to keep track of the next offset in the SAK file.  
		// When a void resource is loaded from a SAK file, it uses the
		// resource name to get the offset where the data begins, then it
		// must use this contianer to find the next offset in the directory
		// so it knows where to stop.  This set is sorted by ascending 
		// offsets so it can easily find the last one.  Also, to avoid
		// checking for a special case of the void resource you want being
		// the last resource in the file and thus having to search for EOF
		// rather than reading to the next offset, this set will put in the
		// tell position for the end of the SAK file.  
		dirOffsets	m_SakDirOffset;

		// This is the RFile that is used for SAK files
		RFile m_rfSak;

		// This store an alternate SAK files (XMas runtime patch)
		RFile m_rfSakAlt;
		// And this will store the name / offset mapping, name beeing the name as expected in FromSak function
		dirMap		m_SakAltDirectory;

		// This is the base pathname to prepend to the resource names
		// when loading a file (not when loading from a SAK file)
		RString m_strBasepath; 

		// This is a temp variable used by the FromSystempath function to 
		// store the last called for pathname.  You must use the RString
		// passed back by FromSystempath before calling it again (where it
		// will be overwritten)
		RString m_strFullpath;

		// Write the SAK file header to the current position in
		// the given RFile.  
		int16_t WriteSakHeader(RFile* prf);

		// Helper function to combine the resource name and the base pathname
		// to load your file.
		char* FromSystempath(RString strResourceName)
			{
			RString strSystemPartial = rspPathToSystem((char*) strResourceName);
			m_strFullpath = m_strBasepath + strSystemPartial;
			// Make sure that the RString is not too long for rspix functions
			ASSERT(m_strFullpath.GetLen() < PATH_MAX);
			return (char*) m_strFullpath;
			}

		// Purge all resources even if reference count is not
		// zero.  This is used only by the destructor to 
		// clean up.
		void FreeAllResources(void);
};


///////////////////////////////////////////////////////////////////////////////
//
// Get a resource.
//
// The resource name can be the name of an embedded file within a RSPiX "SAK"
// file or a RSPiX file spec relative to RResMgr's base path.
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
int16_t rspGetResource(									// Returns 0 on success
	RResMgr*	presmgr,										// In:  Resource Manager to be used
	const char*	pszResName,								// In:  Resource name
	T**	pT,												// Out: Pointer to resource returned here
	RFile::Endian endian = RFile::LittleEndian)	// In:  Endian nature of resource file
	{
	// Create function objects for the specified type.  We have to allocate
	// them using new because if they're just on the stack, they won't exist
	// beyond this function.  Instead, we leave it up to the Get() function
	// to dispose of them when they are no longer needed.  This is very
	// cheesy, and I hope to figure out a better method soon.
	GenericCreateResFunc* pcreate = new CreateResFunc<T>;
	GenericDestroyResFunc* pdestroy = new DestroyResFunc<T>;
	GenericLoadResFunc* pload = new LoadResFunc<T>;

	// Pass everything in generic form to the resource manager
	return presmgr->Get(pszResName, (void**)pT, endian, pcreate, pdestroy, pload);
	}


///////////////////////////////////////////////////////////////////////////////
//
// Release a resource and set the specified pointer to nullptr.
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
void rspReleaseResource(								// Returns 0 on success
	RResMgr*	presmgr,										// In:  Resource Manager to be used
	T** ppres)												// In:  Pointer to resource
	{
	presmgr->Release(*ppres);
	*ppres = nullptr;
	}


///////////////////////////////////////////////////////////////////////////////
//
// Purges a single resource if its reference count was zero, otherwise it just
// does a release.  This is useful in the cases were you know you only wanted
// to use the resource once and its not really being shared, then you can
// free up the memory after its use.
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
bool rspReleaseAndPurgeResource(	// Returns true if it was acutally purged, 
											// false if its reference count prevented it from being purged
	RResMgr* presmgr,					// In:  Resource Manager to purge resource from
	T** ppres)
	{
	bool bPurged = presmgr->ReleaseAndPurge(*ppres);
	*ppres = nullptr;
	return bPurged;
	}


///////////////////////////////////////////////////////////////////////////////
//
// Get a resource instance.  In this case, you get your own copy of the
// specified resource.
//
// The resource name can be the name of an embedded file within a RSPiX "SAK"
// file or a RSPiX file spec relative to RResMgr's base path.
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
int16_t rspGetResourceInstance(							// Returns 0 on success
	RResMgr*	presmgr,										// In:  Resource Manager to be used
	const char*	pszResName,								// In:  Resource name
	T**	pT,												// Out: Pointer to resource returned here
	RFile::Endian endian = RFile::LittleEndian)	// In:  Endian nature of resource file
	{
	// Create function objects for the specified type.  We do NOT have to allocate
	// them using new because they're just needed for a little while.
	CreateResFunc<T>	create;
	DestroyResFunc<T>	destroy;
	LoadResFunc<T>		load;

	// Pass everything in generic form to the resource manager
	return presmgr->GetInstance(pszResName, (void**)pT, endian, &create, &destroy, &load);
	}


///////////////////////////////////////////////////////////////////////////////
//
// Release a resource instance and set the specified pointer to nullptr.  
// The resource will be destroyed immediately.
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
void rspReleaseResourceInstance(						// Returns 0 on success
	RResMgr*	/*presmgr*/,								// In:  Resource Manager to be used
																// DO NOT GET RID OF THIS ARGUMENT
																// It may be needed in the future
																// if we ever decide to track these
																// objects via the resmgr.
	T** ppres)												// In:  Pointer to resource
	{
	// Be gone.
	delete *ppres;
	*ppres = nullptr;
	}

#endif //RESMGR_H

///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////
