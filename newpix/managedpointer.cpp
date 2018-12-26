#include "managedpointer.h"

uint64_t g_insert_count = 0;
uint64_t g_erase_count = 0;
uint64_t g_lookup_count = 0;
uint64_t g_reset_count = 0;
uint64_t g_validity_check_count = 0;

#ifndef NEW_MANAGED_PTR
std::set<void*> g_all_pointers;
#else
std::set<std::shared_ptr<Object>> g_all_pointers;

bool operator ==(const std::shared_ptr<Object> shobj, Object *const obj)
{
  return shobj.get() == obj;
}


managed_object_ptr::managed_object_ptr(Object* ptr) noexcept
  : m_ptr()
{
  if(ptr != nullptr)
  {
    auto iter = std::find(g_all_pointers.begin(), g_all_pointers.end(), ptr);
    if(iter == g_all_pointers.end())
      m_ptr = *g_all_pointers.emplace(ptr).first;
    else
      m_ptr = *iter;
  }
}

managed_object_ptr::operator Object*(void) const noexcept
{
  if(m_ptr.expired())
    return nullptr;
  return m_ptr.lock().get();
}

managed_object_ptr::operator bool(void) const noexcept { return !m_ptr.expired(); }

void managed_object_ptr::reset(void) noexcept { m_ptr.reset(); }

void managed_object_ptr::destroy(Object* ptr) noexcept
{
  auto iter = std::find(g_all_pointers.begin(), g_all_pointers.end(), ptr);
  if(iter == g_all_pointers.end())
    g_all_pointers.erase(iter);
}

#endif
