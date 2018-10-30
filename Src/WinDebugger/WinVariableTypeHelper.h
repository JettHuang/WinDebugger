// \brief
//		build type description block.
//

#pragma once

#include <Windows.h>
#include <DbgHelp.h>

#include <string>
#include <vector>



enum BaseTypeEnum {
	btNoType = 0,
	btVoid = 1,
	btChar = 2,
	btWChar = 3,
	btInt = 6,
	btUInt = 7,
	btFloat = 8,
	btBCD = 9,
	btBool = 10,
	btLong = 13,
	btULong = 14,
	btCurrency = 25,
	btDate = 26,
	btVariant = 27,
	btComplex = 28,
	btBit = 29,
	btBSTR = 30,
	btHresult = 31
};

enum SymTagEnum {
	SymTagNull,
	SymTagExe,
	SymTagCompiland,
	SymTagCompilandDetails,
	SymTagCompilandEnv,
	SymTagFunction,				//函数
	SymTagBlock,
	SymTagData,					//变量
	SymTagAnnotation,
	SymTagLabel,
	SymTagPublicSymbol,
	SymTagUDT,					//用户定义类型，例如struct，class和union
	SymTagEnum,					//枚举类型
	SymTagFunctionType,			//函数类型
	SymTagPointerType,			//指针类型
	SymTagArrayType,				//数组类型
	SymTagBaseType,				//基本类型
	SymTagTypedef,				//typedef类型
	SymTagBaseClass,				//基类
	SymTagFriend,				//友元类型
	SymTagFunctionArgType,		//函数参数类型
	SymTagFuncDebugStart,
	SymTagFuncDebugEnd,
	SymTagUsingNamespace,
	SymTagVTableShape,
	SymTagVTable,
	SymTagCustom,
	SymTagThunk,
	SymTagCustomType,
	SymTagManagedType,
	SymTagDimension
};

// c/c++ primitive type
enum CPrimitiveTypeEnum {
	cbtNone,
	cbtVoid,
	cbtBool,
	cbtChar,
	cbtUChar,
	cbtWChar,
	cbtShort,
	cbtUShort,
	cbtInt,
	cbtUInt,
	cbtLong,
	cbtULong,
	cbtLongLong,
	cbtULongLong,
	cbtFloat,
	cbtDouble,
	cbtEnd,
};

// class type info
class FSymTypeInfo
{
public:
	FSymTypeInfo() {}
	virtual ~FSymTypeInfo() {}

	// get type name
	virtual std::wstring TypeName() const = 0;
	// get format value
	virtual std::wstring FormatValue(void *ValuePtr) const = 0;
};

class FSymUnknownType : public FSymTypeInfo
{
public:
	~FSymUnknownType();

	static FSymUnknownType* StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId);

	// get type name
	virtual std::wstring TypeName() const override;
	// get format value
	virtual std::wstring FormatValue(void *ValuePtr) const override;
protected:
	FSymUnknownType();
};


class FSymPrimitiveType : public FSymTypeInfo
{
public:
	~FSymPrimitiveType();

	static FSymPrimitiveType* StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId);

	// get type name
	virtual std::wstring TypeName() const override;
	// get format value
	virtual std::wstring FormatValue(void *ValuePtr) const override;
protected:
	FSymPrimitiveType();

	CPrimitiveTypeEnum	PrimitiveType;
};

// pointer type
class FSymPointerType : public FSymTypeInfo
{
public:
	~FSymPointerType();

	static FSymPointerType* StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId);

	// get type name
	virtual std::wstring TypeName() const override;
	// get format value
	virtual std::wstring FormatValue(void *ValuePtr) const override;
protected:
	FSymPointerType();

	FSymTypeInfo    *pInnerType;  // the pointed data type.
	bool			bIsReference; // is & ?
};

// array type
class FSymArrayType : public FSymTypeInfo
{
public:
	~FSymArrayType();

	static FSymArrayType* StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId);

	// get type name
	virtual std::wstring TypeName() const override;
	// get format value
	virtual std::wstring FormatValue(void *ValuePtr) const override;
protected:
	FSymArrayType();

	FSymTypeInfo    *pInnerType;  // the pointed data type.
	uint32_t		 ElementCount;// element count
	uint32_t		 ElementLength; // bytes per element
};

// enum type
class FSymEnumType : public FSymTypeInfo
{
public:
	~FSymEnumType();

	static FSymEnumType* StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId);

	// get type name
	virtual std::wstring TypeName() const override;
	// get format value
	virtual std::wstring FormatValue(void *ValuePtr) const override;
protected:
	FSymEnumType();

	struct FEnumElement
	{
		std::wstring   Name;
		VARIANT		   Value;
	};

	CPrimitiveTypeEnum			ValueType;
	std::wstring				szTypeName;
	std::vector<FEnumElement>	EnumValues;
};

// function type
class FSymFunctionType : public FSymTypeInfo
{
public:
	~FSymFunctionType();

	static FSymFunctionType* StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId);

	// get type name
	virtual std::wstring TypeName() const override;
	// get format value
	virtual std::wstring FormatValue(void *ValuePtr) const override;
protected:
	FSymFunctionType();

	FSymTypeInfo			   *pReturnType;
	std::vector<FSymTypeInfo*>  ParamTypes;
	std::wstring				szFunctionProto;
};

// typedef type
class FSymTypedefType : public FSymTypeInfo
{
public:
	~FSymTypedefType();

	static FSymTypedefType* StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId);

	// get type name
	virtual std::wstring TypeName() const override;
	// get format value
	virtual std::wstring FormatValue(void *ValuePtr) const override;
protected:
	FSymTypedefType();

	std::wstring	DefName;
	FSymTypeInfo    *pInnerType;  // the real type
};

// user define type(class, union, struct)
class FSymComplexType : public FSymTypeInfo
{
public:
	~FSymComplexType();

	static FSymComplexType* StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId);

	// get type name
	virtual std::wstring TypeName() const override;
	// get format value
	virtual std::wstring FormatValue(void *ValuePtr) const override;
protected:
	FSymComplexType();

	struct FMemberElement
	{
		std::wstring	Name;
		uint32_t		Offset;
		FSymTypeInfo	*pTypeInfo;
	};

	std::wstring					UserTypeName;
	std::vector<FMemberElement>		Members;
};

// build symbol type description
class FSymTypeInfoHelper
{
public:
	static void Initialize();
	static void Uninitialize();
	static void CacheSymTypeInfo(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId, FSymTypeInfo *InTypeInfo);
	static FSymTypeInfo* BuildSymTypeInfo(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId);
};

