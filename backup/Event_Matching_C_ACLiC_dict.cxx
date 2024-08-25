// Do NOT change. Changes will be lost next time file is generated

#define R__DICTIONARY_FILENAME dIUsersdIishitanisoshidIkalliopedIanadIEvent_Matching_C_ACLiC_dict
#define R__NO_DEPRECATION

/*******************************************************************/
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define G__DICTIONARY
#include "ROOT/RConfig.hxx"
#include "TClass.h"
#include "TDictAttributeMap.h"
#include "TInterpreter.h"
#include "TROOT.h"
#include "TBuffer.h"
#include "TMemberInspector.h"
#include "TInterpreter.h"
#include "TVirtualMutex.h"
#include "TError.h"

#ifndef G__ROOT
#define G__ROOT
#endif

#include "RtypesImp.h"
#include "TIsAProxy.h"
#include "TFileMergeInfo.h"
#include <algorithm>
#include "TCollectionProxyInfo.h"
/*******************************************************************/

#include "TDataMember.h"

// The generated code does not explicitly qualify STL entities
namespace std {} using namespace std;

// Header files passed as explicit arguments
#include "/Users/ishitanisoshi/kalliope/ana/./Event_Matching.C"

// Header files passed via #pragma extra_include

namespace ROOT {
   static TClass *Event_Matching_Dictionary();
   static void Event_Matching_TClassManip(TClass*);
   static void *new_Event_Matching(void *p = nullptr);
   static void *newArray_Event_Matching(Long_t size, void *p);
   static void delete_Event_Matching(void *p);
   static void deleteArray_Event_Matching(void *p);
   static void destruct_Event_Matching(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::Event_Matching*)
   {
      ::Event_Matching *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TIsAProxy(typeid(::Event_Matching));
      static ::ROOT::TGenericClassInfo 
         instance("Event_Matching", "Event_Matching.h", 18,
                  typeid(::Event_Matching), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &Event_Matching_Dictionary, isa_proxy, 4,
                  sizeof(::Event_Matching) );
      instance.SetNew(&new_Event_Matching);
      instance.SetNewArray(&newArray_Event_Matching);
      instance.SetDelete(&delete_Event_Matching);
      instance.SetDeleteArray(&deleteArray_Event_Matching);
      instance.SetDestructor(&destruct_Event_Matching);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::Event_Matching*)
   {
      return GenerateInitInstanceLocal(static_cast<::Event_Matching*>(nullptr));
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const ::Event_Matching*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));

   // Dictionary for non-ClassDef classes
   static TClass *Event_Matching_Dictionary() {
      TClass* theClass =::ROOT::GenerateInitInstanceLocal(static_cast<const ::Event_Matching*>(nullptr))->GetClass();
      Event_Matching_TClassManip(theClass);
   return theClass;
   }

   static void Event_Matching_TClassManip(TClass* theClass){
      theClass->CreateAttributeMap();
      TDictAttributeMap* attrMap( theClass->GetAttributeMap() );
      attrMap->AddProperty("file_name","/Users/ishitanisoshi/kalliope/ana/./Event_Matching.h");
   }

} // end of namespace ROOT

namespace ROOT {
   // Wrappers around operator new
   static void *new_Event_Matching(void *p) {
      return  p ? new(p) ::Event_Matching : new ::Event_Matching;
   }
   static void *newArray_Event_Matching(Long_t nElements, void *p) {
      return p ? new(p) ::Event_Matching[nElements] : new ::Event_Matching[nElements];
   }
   // Wrapper around operator delete
   static void delete_Event_Matching(void *p) {
      delete (static_cast<::Event_Matching*>(p));
   }
   static void deleteArray_Event_Matching(void *p) {
      delete [] (static_cast<::Event_Matching*>(p));
   }
   static void destruct_Event_Matching(void *p) {
      typedef ::Event_Matching current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class ::Event_Matching

namespace ROOT {
   static TClass *vectorlEdoublegR_Dictionary();
   static void vectorlEdoublegR_TClassManip(TClass*);
   static void *new_vectorlEdoublegR(void *p = nullptr);
   static void *newArray_vectorlEdoublegR(Long_t size, void *p);
   static void delete_vectorlEdoublegR(void *p);
   static void deleteArray_vectorlEdoublegR(void *p);
   static void destruct_vectorlEdoublegR(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const vector<double>*)
   {
      vector<double> *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TIsAProxy(typeid(vector<double>));
      static ::ROOT::TGenericClassInfo 
         instance("vector<double>", -2, "vector", 348,
                  typeid(vector<double>), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &vectorlEdoublegR_Dictionary, isa_proxy, 0,
                  sizeof(vector<double>) );
      instance.SetNew(&new_vectorlEdoublegR);
      instance.SetNewArray(&newArray_vectorlEdoublegR);
      instance.SetDelete(&delete_vectorlEdoublegR);
      instance.SetDeleteArray(&deleteArray_vectorlEdoublegR);
      instance.SetDestructor(&destruct_vectorlEdoublegR);
      instance.AdoptCollectionProxyInfo(TCollectionProxyInfo::Generate(TCollectionProxyInfo::Pushback< vector<double> >()));

      instance.AdoptAlternate(::ROOT::AddClassAlternate("vector<double>","std::__1::vector<double, std::__1::allocator<double>>"));
      return &instance;
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const vector<double>*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));

   // Dictionary for non-ClassDef classes
   static TClass *vectorlEdoublegR_Dictionary() {
      TClass* theClass =::ROOT::GenerateInitInstanceLocal(static_cast<const vector<double>*>(nullptr))->GetClass();
      vectorlEdoublegR_TClassManip(theClass);
   return theClass;
   }

   static void vectorlEdoublegR_TClassManip(TClass* ){
   }

} // end of namespace ROOT

namespace ROOT {
   // Wrappers around operator new
   static void *new_vectorlEdoublegR(void *p) {
      return  p ? ::new((::ROOT::Internal::TOperatorNewHelper*)p) vector<double> : new vector<double>;
   }
   static void *newArray_vectorlEdoublegR(Long_t nElements, void *p) {
      return p ? ::new((::ROOT::Internal::TOperatorNewHelper*)p) vector<double>[nElements] : new vector<double>[nElements];
   }
   // Wrapper around operator delete
   static void delete_vectorlEdoublegR(void *p) {
      delete (static_cast<vector<double>*>(p));
   }
   static void deleteArray_vectorlEdoublegR(void *p) {
      delete [] (static_cast<vector<double>*>(p));
   }
   static void destruct_vectorlEdoublegR(void *p) {
      typedef vector<double> current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class vector<double>

namespace {
  void TriggerDictionaryInitialization_Event_Matching_C_ACLiC_dict_Impl() {
    static const char* headers[] = {
"./Event_Matching.C",
nullptr
    };
    static const char* includePaths[] = {
"/usr/local/root/include",
"/usr/local/root/etc/",
"/usr/local/root/etc//cling",
"/usr/local/root/etc//cling/plugins/include",
"/usr/local/root/include/",
"/usr/local/root/include",
"/usr/local/include",
"/usr/local/anaroot/include",
"/System/Volumes/Data/build/ws/BUILDTYPE/Release/LABEL/mac13/V/6-28/build/include",
"/System/Volumes/Data/build/ws/BUILDTYPE/Release/LABEL/mac13/V/6-28/build/include/",
"/Users/ishitanisoshi/Downloads/anaroot/sources/Nadeko/",
"/Users/ishitanisoshi/Downloads/anaroot/sources/Core/",
"../../Core/include",
"/Users/ishitanisoshi/Downloads/anaroot/sources/Reconstruction/BigRIPS/",
"../../Reconstruction/BigRIPS/include",
"../../Reconstruction/SAMURAI/include",
"/Users/ishitanisoshi/Downloads/anaroot/sources/Reconstruction/DALI/",
"/Users/ishitanisoshi/Downloads/anaroot/sources/Reconstruction/SAMURAI/",
"/Users/ishitanisoshi/Downloads/anaroot/sources/Reconstruction/CATANA/",
"/Users/ishitanisoshi/Downloads/anaroot/sources/Reconstruction/ESPRI/",
"/Users/ishitanisoshi/Downloads/anaroot/sources/Reconstruction/WINDS/",
"../../Nadeko/include",
"/Users/ishitanisoshi/Downloads/anaroot/sources/AnaLoop/Core/",
"../Core/include",
"../../Reconstruction/CATANA/include",
"../../Reconstruction/DALI/include",
"../../Reconstruction/ESPRI/include",
"../../Reconstruction/MINOS/include",
"../../Reconstruction/SILICONS/include",
"../../Reconstruction/WINDS/include",
"/Users/ishitanisoshi/Downloads/anaroot/sources/AnaLoop/Example/",
"/usr/local/root/include/",
"/Users/ishitanisoshi/kalliope/ana/",
nullptr
    };
    static const char* fwdDeclCode = R"DICTFWDDCLS(
#line 1 "Event_Matching_C_ACLiC_dict dictionary forward declarations' payload"
#pragma clang diagnostic ignored "-Wkeyword-compat"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
extern int __Cling_AutoLoading_Map;
class __attribute__((annotate("$clingAutoload$Event_Matching.h")))  __attribute__((annotate("$clingAutoload$./Event_Matching.C")))  Event_Matching;
)DICTFWDDCLS";
    static const char* payloadCode = R"DICTPAYLOAD(
#line 1 "Event_Matching_C_ACLiC_dict dictionary payload"

#ifndef __ACLIC__
  #define __ACLIC__ 1
#endif

#define _BACKWARD_BACKWARD_WARNING_H
// Inline headers
#include "./Event_Matching.C"

#undef  _BACKWARD_BACKWARD_WARNING_H
)DICTPAYLOAD";
    static const char* classesHeaders[] = {
"", payloadCode, "@",
"Event_Matching", payloadCode, "@",
nullptr
};
    static bool isInitialized = false;
    if (!isInitialized) {
      TROOT::RegisterModule("Event_Matching_C_ACLiC_dict",
        headers, includePaths, payloadCode, fwdDeclCode,
        TriggerDictionaryInitialization_Event_Matching_C_ACLiC_dict_Impl, {}, classesHeaders, /*hasCxxModule*/false);
      isInitialized = true;
    }
  }
  static struct DictInit {
    DictInit() {
      TriggerDictionaryInitialization_Event_Matching_C_ACLiC_dict_Impl();
    }
  } __TheDictionaryInitializer;
}
void TriggerDictionaryInitialization_Event_Matching_C_ACLiC_dict() {
  TriggerDictionaryInitialization_Event_Matching_C_ACLiC_dict_Impl();
}
