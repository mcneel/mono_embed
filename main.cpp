#include <string>
#include <dlfcn.h>
#include <stdio.h>

struct MonoDomain;
struct MonoAssembly;
struct MonoImage;
struct MonoClass;
struct MonoMethod;
struct MonoObject;


// Mono types/functions used for embedding
typedef void (__cdecl *PFUNC_mono_set_dirs)(const char*, const char*);
typedef void (__cdecl *PFUNC_mono_onestring)(const char*);
typedef MonoDomain* (__cdecl *PFUNC_mono_jit_init_version)(const char*, const char*);
typedef MonoAssembly* (__cdecl *PFUNC_mono_domain_assembly_open)(MonoDomain*, const char*);
typedef MonoImage* (__cdecl *PFUNC_mono_assembly_get_image)(MonoAssembly*);
typedef MonoClass* (__cdecl *PFUNC_mono_class_from_name)(MonoImage*, const char*, const char*);
typedef MonoMethod* (__cdecl *PFUNC_mono_class_get_method_from_name)( MonoClass*, const char*, int);
typedef MonoObject* (__cdecl *PFUNC_mono_runtime_invoke)( MonoMethod*, void*, void**, MonoObject** );

static PFUNC_mono_jit_init_version            mono_jit_init_version = nullptr;
static PFUNC_mono_set_dirs                    mono_set_dirs = nullptr;
static PFUNC_mono_onestring                   mono_trace_set_level_string = nullptr;
static PFUNC_mono_domain_assembly_open        mono_domain_assembly_open = nullptr;
static PFUNC_mono_assembly_get_image          mono_assembly_get_image   = nullptr;
static PFUNC_mono_class_from_name             mono_class_from_name      = nullptr;
static PFUNC_mono_class_get_method_from_name  mono_class_get_method_from_name = nullptr;
static PFUNC_mono_runtime_invoke              mono_runtime_invoke = nullptr;



static bool InitializeMonoForOSX(const char* frameworkBasePath)
{
  // Dynamically load Mono.framework
  const std::string base_path = frameworkBasePath;
  const std::string assembly_dir = base_path + "/lib";
  const std::string config_dir = base_path + "/etc";
  const std::string mono_dylib = base_path + "/lib/libmonosgen-2.0.dylib";

  void* mono_framework_handle = dlopen(mono_dylib.c_str(), RTLD_LAZY);
  if (nullptr == mono_framework_handle)
  {
    // for debugging
    // char* load_error = dlerror();
    return false;
  }
  
  
  mono_set_dirs = (PFUNC_mono_set_dirs) dlsym(RTLD_DEFAULT, "mono_set_dirs");
  mono_trace_set_level_string = (PFUNC_mono_onestring) dlsym(RTLD_DEFAULT, "mono_trace_set_level_string");
  mono_jit_init_version = (PFUNC_mono_jit_init_version)dlsym(RTLD_DEFAULT, "mono_jit_init_version");
  mono_domain_assembly_open = (PFUNC_mono_domain_assembly_open) dlsym(RTLD_DEFAULT, "mono_domain_assembly_open");
  mono_assembly_get_image = (PFUNC_mono_assembly_get_image) dlsym(RTLD_DEFAULT, "mono_assembly_get_image");
  mono_class_from_name = (PFUNC_mono_class_from_name) dlsym(RTLD_DEFAULT, "mono_class_from_name");
  mono_class_get_method_from_name = (PFUNC_mono_class_get_method_from_name) dlsym(RTLD_DEFAULT, "mono_class_get_method_from_name");
  mono_runtime_invoke = (PFUNC_mono_runtime_invoke) dlsym(RTLD_DEFAULT, "mono_runtime_invoke");

  bool mono_framework_installed = nullptr != mono_set_dirs &&
                                  nullptr != mono_jit_init_version &&
                                  nullptr != mono_domain_assembly_open &&
                                  nullptr != mono_assembly_get_image &&
                                  nullptr != mono_class_from_name &&
                                  nullptr != mono_class_get_method_from_name &&
                                  nullptr != mono_runtime_invoke;
  
  if (!mono_framework_installed)
    return false;
  
  mono_set_dirs(assembly_dir.c_str(), config_dir.c_str());
  
  // let mono spew out as much information as possible
  if( mono_trace_set_level_string )
    mono_trace_set_level_string("debug");
  
  return true;
}


int main (int argc, char* argv[])
{
  printf("Starting embed test\n");

  // using lame hard coded paths because I'm lazy and just want to get a sample put together
  auto mono_path = "/Users/stevenbaer/dev/rhino/mac5/src4/rhino4/MacOS/Mono64Framework/Mono64Rhino.framework/Versions/Current";
  auto assembly_path = "/Users/stevenbaer/dev/mono_embed/EmbedTest/bin/Debug/EmbedTest.exe";
  
  if( !InitializeMonoForOSX(mono_path) )
    return 1;
  
  MonoDomain* app_domain = mono_jit_init_version(assembly_path, "v4.0");
  if( nullptr == app_domain )
    return 1;
 
  MonoAssembly* assembly = mono_domain_assembly_open( app_domain, assembly_path);
  if( nullptr == assembly )
    return 1;
  
  MonoImage* mono_image = mono_assembly_get_image(assembly);
  if( nullptr == mono_image )
    return 1;
  
  MonoClass* mono_class = mono_class_from_name(mono_image, "EmbedTest", "MainClass");
  if( nullptr == mono_class )
    return 1;
  
  MonoMethod* method = mono_class_get_method_from_name( mono_class, "CallbackTest", -1 );
  if( nullptr == method )
    return 1;
  
  mono_runtime_invoke(method, nullptr, nullptr, nullptr);
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Exported callbacks
#define EXPORTED_FUNCTION extern "C" __attribute__ ((visibility ("default")))
typedef unsigned short                  unichar;


EXPORTED_FUNCTION void CallbackA(int i, double d)
{
  double x = i+d;
  printf("CallbackA %f\n", x);
}

EXPORTED_FUNCTION void CallbackB(const unichar* s)
{
  if( nullptr==s )
    return;

  int len = 0;
  for( len=0; s[len]!=0; len++);
  wchar_t* str = new wchar_t[len+1];
  for( int i=0; i<len; i++ )
    str[i] = s[i];
  str[len] = 0;
  
  printf("CallbackB %S\n", str);
  delete[] str;
}
