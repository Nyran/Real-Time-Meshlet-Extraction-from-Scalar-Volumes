#ifndef FRAMEWORK_PLATFORM_HPP
#define FRAMEWORK_PLATFORM_HPP

//see:  https://stackoverflow.com/questions/2164827/explicitly-exporting-shared-library-functions-in-linux
#if defined(_MSC_VER)
//  Microsoft 
#define FRAMEWORK_DLL_EXPORT __declspec(dllexport)
#define FRAMEWORL_DLL_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
//  GCC
#define FRAMEWORK_DLL_EXPORT __attribute__((visibility("default")))
#define FRAMEWORL_DLL_IMPORT
#else
//  do nothing and hope for the best?
#define FRAMEWORK_DLL_EXPORT
#define FRAMEWORL_DLL_IMPORT	
#pragma warning Unknown dynamic link import/export semantics.
#endif

#define GLM_ENABLE_EXPERIMENTAL

#endif //FRAMEWORK_PLATFORM_HPP