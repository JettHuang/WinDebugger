// \brief
//		build debug symbol descriptions 
//

#include "WinVariableTypeHelper.h"
#include "Foundation/AppHelper.h"

#include <sstream>
#include <iomanip>
#include <map>


//将char类型的字符转换成可以输出到控制台的字符,
//如果ch是不可显示的字符，则返回一个问号，
//否则直接返回ch。
//小于0x1E和大于0x7F的值都不能显示。
static char ConvertToSafeChar(char ch) {

	if (ch < 0x1E || ch > 0x7F) {
		return '?';
	}

	return ch;
}



//将wchar_t类型的字符转换成可以输出到控制台的字符,
//如果当前的代码页不能显示ch，则返回一个问号，
//否则直接返回ch。
static wchar_t ConvertToSafeWChar(wchar_t ch) {

	if (ch < 0x1E) {
		return L'?';
	}

	char buffer[4];

	size_t convertedCount;
	wcstombs_s(&convertedCount, buffer, 4, &ch, 2);

	if (convertedCount == 0) {
		return L'?';
	}

	return ch;
}

static const TCHAR* GetPrimitiveTypeText(uint32_t InType)
{
	struct FPrimitveTypeDesc
	{
		uint32_t   TypeVal;
		const TCHAR *szText;
	};

	static FPrimitveTypeDesc sTypes[] =
	{
		{ cbtNone, TEXT("<no-type>") },
		{ cbtVoid, TEXT("void") },
		{ cbtBool, TEXT("bool") },
		{ cbtChar, TEXT("char") },
		{ cbtUChar, TEXT("unsigned char") },
		{ cbtWChar, TEXT("wchar_t") },
		{ cbtShort, TEXT("short") },
		{ cbtUShort, TEXT("unsigned short") },
		{ cbtInt, TEXT("int") },
		{ cbtUInt, TEXT("unsigned int") },
		{ cbtLong, TEXT("long") },
		{ cbtULong, TEXT("unsigned long") },
		{ cbtLongLong, TEXT("long long") },
		{ cbtULongLong, TEXT("unsigned long long") },
		{ cbtFloat, TEXT("float") },
		{ cbtDouble, TEXT("double") },
		{ cbtEnd, TEXT("") }
	};

	for (uint32_t k=0; k<XARRAY_COUNT(sTypes); k++)
	{
		if (sTypes[k].TypeVal == InType)
		{
			return sTypes[k].szText;
		}
	} // end for k

	return TEXT("UnKnown");
}

static CPrimitiveTypeEnum TranslateBaseTypeToC(DWORD BaseType, ULONG64 Length)
{
	CPrimitiveTypeEnum cType = cbtNone;

	switch (BaseType)
	{
	case btVoid:
		cType = cbtVoid;
		break;
	case btChar:
		cType = cbtChar;
		break;
	case btWChar:
		cType = cbtWChar;
		break;
	case btInt:
		switch (Length)
		{
		case 2:  cType = cbtShort; break;
		case 4:  cType = cbtInt; break;
		default: cType = cbtLongLong;
		}
		break;
	case btUInt:
		switch (Length) {
		case 1:  cType = cbtUChar; break;
		case 2:  cType = cbtUShort; break;
		case 4:  cType = cbtUInt; break;
		default: cType = cbtULongLong;
		}
		break;
	case btFloat:
		switch (Length) {
		case 4:  cType = cbtFloat; break;
		default: cType = cbtDouble;
		}
		break;
	case btBool:
		cType = cbtBool;
		break;
	case btLong:
		cType = cbtLong;
		break;
	case btULong:
		cType = cbtULong;
		break;
	default:
		cType = cbtNone;
	}

	return cType;
}

static std::wstring FormatPrimitiveTypeValue(CPrimitiveTypeEnum PrimitiveType, void *pData)
{
	std::wostringstream valueBuilder;

	switch (PrimitiveType) {

	case cbtNone:
		valueBuilder << TEXT("??");
		break;

	case cbtVoid:
		valueBuilder << TEXT("??");
		break;

	case cbtBool:
		valueBuilder << (*(char*)pData == 0 ? L"false" : L"true");
		break;

	case cbtChar:
		valueBuilder << ConvertToSafeChar(*((char*)pData));
		break;

	case cbtUChar:
		valueBuilder << std::hex
			<< std::uppercase
			<< std::setw(2)
			<< std::setfill(TEXT('0'))
			<< *((unsigned char*)pData);
		break;

	case cbtWChar:
		valueBuilder << ConvertToSafeWChar(*((wchar_t*)pData));
		break;

	case cbtShort:
		valueBuilder << *((short*)pData);
		break;

	case cbtUShort:
		valueBuilder << *((unsigned short*)pData);
		break;

	case cbtInt:
		valueBuilder << *((int*)pData);
		break;

	case cbtUInt:
		valueBuilder << *((unsigned int*)pData);
		break;

	case cbtLong:
		valueBuilder << *((long*)pData);
		break;

	case cbtULong:
		valueBuilder << *((unsigned long*)pData);
		break;

	case cbtLongLong:
		valueBuilder << *((long long*)pData);
		break;

	case cbtULongLong:
		valueBuilder << *((unsigned long long*)pData);
		break;

	case cbtFloat:
		valueBuilder << *((float*)pData);
		break;

	case cbtDouble:
		valueBuilder << *((double*)pData);
		break;
	}

	return valueBuilder.str();
}

static bool VariantEqual(const VARIANT &var, CPrimitiveTypeEnum cBaseType, const void* pData) {

	switch (cBaseType) 
	{
	case cbtChar:
		return var.cVal == *((char*)pData);

	case cbtUChar:
		return var.bVal == *((unsigned char*)pData);

	case cbtShort:
		return var.iVal == *((short*)pData);

	case cbtWChar:
	case cbtUShort:
		return var.uiVal == *((unsigned short*)pData);

	case cbtUInt:
		return var.uintVal == *((int*)pData);

	case cbtLong:
		return var.lVal == *((long*)pData);

	case cbtULong:
		return var.ulVal == *((unsigned long*)pData);

	case cbtLongLong:
		return var.llVal == *((long long*)pData);

	case cbtULongLong:
		return var.ullVal == *((unsigned long long*)pData);

	case cbtInt:
	default:
		return var.intVal == *((int*)pData);
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////

FSymUnknownType::FSymUnknownType()
{
}

FSymUnknownType::~FSymUnknownType()
{
}

FSymUnknownType* FSymUnknownType::StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId)
{
	FSymUnknownType *pNew = new FSymUnknownType();

	if (pNew)
	{
		FSymTypeInfoHelper::CacheSymTypeInfo(InProcess, InModuleBase, TypeId, pNew);
	}

	return pNew;
}

// get type name
std::wstring FSymUnknownType::TypeName() const
{
	return TEXT("Unknown");
}

// get format value
std::wstring FSymUnknownType::FormatValue(void *pData) const
{
	std::wostringstream valueBuilder;

	valueBuilder  << std::hex
		<< std::uppercase
		<< std::setw(8)
		<< std::setfill(TEXT('0'))
		<< *((DWORD*)pData);

	return valueBuilder.str();
}

//////////////////////////////////////////////////////////////////////////

FSymPrimitiveType::FSymPrimitiveType()
{
}

FSymPrimitiveType::~FSymPrimitiveType()
{
}


FSymPrimitiveType* FSymPrimitiveType::StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId)
{
	DWORD BaseType = 0;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_BASETYPE, &BaseType);

	ULONG64 Length = 0;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_LENGTH, &Length);

	FSymPrimitiveType* pNew = new FSymPrimitiveType();
	if (pNew)
	{
		FSymTypeInfoHelper::CacheSymTypeInfo(InProcess, InModuleBase, TypeId, pNew);

		pNew->PrimitiveType = TranslateBaseTypeToC(BaseType, Length);
	}

	return pNew;
}

std::wstring FSymPrimitiveType::TypeName() const
{
	return std::wstring(GetPrimitiveTypeText(PrimitiveType));
}

// get format value
std::wstring FSymPrimitiveType::FormatValue(void *pData) const
{
	return FormatPrimitiveTypeValue(PrimitiveType, pData);
}

//////////////////////////////////////////////////////////////////////////

FSymPointerType::FSymPointerType()
	: pInnerType(NULL)
	, bIsReference(false)
{

}

FSymPointerType::~FSymPointerType()
{
	if (pInnerType)
	{
		delete pInnerType; pInnerType = NULL;
	}
}

FSymPointerType* FSymPointerType::StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId)
{
	BOOL IsReference = FALSE;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_IS_REFERENCE, &IsReference);

	DWORD InnerTypeId;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_TYPEID, &InnerTypeId);

	FSymPointerType *pNew = new FSymPointerType();
	if (pNew)
	{
		FSymTypeInfoHelper::CacheSymTypeInfo(InProcess, InModuleBase, TypeId, pNew);

		pNew->bIsReference = !!IsReference;
		pNew->pInnerType = FSymTypeInfoHelper::BuildSymTypeInfo(InProcess, InModuleBase, InnerTypeId);
	}

	return pNew;
}

// get type name
std::wstring FSymPointerType::TypeName() const
{
	std::wstring szName(TEXT("Unknown"));

	if (pInnerType)
	{
		szName = pInnerType->TypeName();
	}

	szName += bIsReference ? TEXT("&") : TEXT("*");
	return szName;
}

// get format value
std::wstring FSymPointerType::FormatValue(void *pData) const
{
	std::wostringstream valueBuilder;

	valueBuilder << std::hex << std::uppercase << std::setfill(TEXT('0')) << std::setw(8) << *((DWORD*)pData);

	return valueBuilder.str();
}

//////////////////////////////////////////////////////////////////////////

FSymArrayType::FSymArrayType()
	: pInnerType(NULL)
	, ElementCount(0)
	, ElementLength(0)
{
}

FSymArrayType::~FSymArrayType()
{
	if (pInnerType)
	{
		delete pInnerType; pInnerType = NULL;
	}
}

FSymArrayType* FSymArrayType::StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId)
{
	DWORD ElemCount;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_COUNT, &ElemCount);

	DWORD InnerTypeId;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_TYPEID, &InnerTypeId);

	ULONG64 InnerLength;
	SymGetTypeInfo(InProcess, InModuleBase, InnerTypeId, TI_GET_LENGTH, &InnerLength);

	FSymArrayType *pNew = new FSymArrayType();
	if (pNew)
	{
		FSymTypeInfoHelper::CacheSymTypeInfo(InProcess, InModuleBase, TypeId, pNew);

		pNew->ElementCount = ElemCount;
		pNew->ElementLength = (uint32_t)InnerLength;
		pNew->pInnerType = FSymTypeInfoHelper::BuildSymTypeInfo(InProcess, InModuleBase, InnerTypeId);
	}

	return pNew;
}

// get type name
std::wstring FSymArrayType::TypeName() const
{
	std::wostringstream strBuilder;

	if (pInnerType)
	{
		strBuilder << pInnerType->TypeName() << TEXT("[") << ElementCount << TEXT("]");
	}
	else
	{
		strBuilder << TEXT("Unknown") << TEXT("[") << ElementCount << TEXT("]");
	}

	return strBuilder.str();
}

// get format value
std::wstring FSymArrayType::FormatValue(void *ValuePtr) const
{
	std::wostringstream valueBuilder;

	if (pInnerType)
	{
		uint32_t Count = ElementCount;
		if (Count > 32) { Count = 32; }

		valueBuilder << std::endl;
		for (uint32_t k=0; k<Count; k++)
		{
			valueBuilder << TEXT("[") << k << TEXT("]: ") << pInnerType->FormatValue(((byte *)ValuePtr) + k * ElementLength)
				<< std::endl;
		} // end for k
	}

	return valueBuilder.str();
}

//////////////////////////////////////////////////////////////////////////

FSymEnumType::FSymEnumType()
{

}

FSymEnumType::~FSymEnumType()
{

}

FSymEnumType* FSymEnumType::StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId)
{
	std::wstring EnumName;

	// 枚举类型名
	TCHAR *pSymName = NULL;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_SYMNAME, &pSymName);
	EnumName = pSymName;
	LocalFree(pSymName);

	// 值类型
	DWORD BaseType = 0;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_BASETYPE, &BaseType);
	ULONG64 Length = 0;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_LENGTH, &Length);

	CPrimitiveTypeEnum PrimType = TranslateBaseTypeToC(BaseType, Length);
	std::vector<FEnumElement>	EnumValues;

	//获取枚举值的个数
	DWORD ChildrenCount;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_CHILDRENCOUNT, &ChildrenCount);

	//获取每个枚举值
	TI_FINDCHILDREN_PARAMS *pFindParams = (TI_FINDCHILDREN_PARAMS *)malloc(sizeof(TI_FINDCHILDREN_PARAMS) + ChildrenCount * sizeof(ULONG));
	pFindParams->Start = 0;
	pFindParams->Count = ChildrenCount;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_FINDCHILDREN, pFindParams);

	for (DWORD k=0; k<ChildrenCount; k++)
	{
		VARIANT enumValue;
		TCHAR *enumName = NULL;

		SymGetTypeInfo(InProcess, InModuleBase, pFindParams->ChildId[k], TI_GET_VALUE, &enumValue);
		SymGetTypeInfo(InProcess, InModuleBase, pFindParams->ChildId[k], TI_GET_SYMNAME, &enumName);

		FEnumElement Entry;
		Entry.Name = enumName;
		Entry.Value = enumValue;

		EnumValues.push_back(Entry);
		LocalFree(enumName);
	} // end for k

	free(pFindParams);

	FSymEnumType *pNew = new FSymEnumType();
	if (pNew)
	{
		FSymTypeInfoHelper::CacheSymTypeInfo(InProcess, InModuleBase, TypeId, pNew);

		pNew->szTypeName = EnumName;
		pNew->ValueType = PrimType;
		pNew->EnumValues = EnumValues;
	}

	return pNew;
}

// get type name
std::wstring FSymEnumType::TypeName() const
{
	return szTypeName;
}

// get format value
std::wstring FSymEnumType::FormatValue(void *ValuePtr) const
{
	for (size_t k=0; k<EnumValues.size(); k++)
	{
		const FEnumElement &Entry = EnumValues[k];
		if (VariantEqual(Entry.Value, ValueType, ValuePtr))
		{
			return Entry.Name;
		}
	} // end for k

	return TEXT("N/A");
}

//////////////////////////////////////////////////////////////////////////

FSymFunctionType::FSymFunctionType()
	: pReturnType(NULL)
{
}

FSymFunctionType::~FSymFunctionType()
{
	if (pReturnType) 
	{ 
		delete pReturnType; pReturnType = NULL; 
	}
	for (size_t k=0; k<ParamTypes.size(); k++)
	{
		if (ParamTypes[k]) 
		{ 
			delete ParamTypes[k]; 
		}
	} // end for k

	ParamTypes.clear();
}

FSymFunctionType* FSymFunctionType::StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId)
{
	FSymFunctionType *pNew = new FSymFunctionType();
	if (!pNew)
	{
		return NULL;
	}
	FSymTypeInfoHelper::CacheSymTypeInfo(InProcess, InModuleBase, TypeId, pNew);

	//获取返回值的名称
	FSymTypeInfo *pReturnType = NULL;
	{
		DWORD returnTypeID;
		SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_TYPEID, &returnTypeID);

		pReturnType = FSymTypeInfoHelper::BuildSymTypeInfo(InProcess, InModuleBase, returnTypeID);
	}

	std::vector<FSymTypeInfo *> ParamTypes;
	{
		//获取参数数量
		DWORD paramCount;
		SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_CHILDRENCOUNT, &paramCount);

		//获取每个参数的类型名称
		BYTE* pBuffer = (BYTE*)malloc(sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG) * paramCount);
		TI_FINDCHILDREN_PARAMS* pFindParams = (TI_FINDCHILDREN_PARAMS*)pBuffer;
		pFindParams->Count = paramCount;
		pFindParams->Start = 0;

		SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_FINDCHILDREN, pFindParams);
		for (DWORD k = 0; k < paramCount; k++)
		{
			DWORD paramTypeID;
			SymGetTypeInfo(InProcess, InModuleBase, pFindParams->ChildId[k], TI_GET_TYPEID, &paramTypeID);

			FSymTypeInfo *ParamType = FSymTypeInfoHelper::BuildSymTypeInfo(InProcess, InModuleBase, paramTypeID);
			ParamTypes.push_back(ParamType);
		} // end for k
		free(pBuffer);
	}

	std::wostringstream nameBuilder;
	nameBuilder << (pReturnType ? pReturnType->TypeName() : TEXT("??"));
	nameBuilder << TEXT("(");
	for (DWORD k = 0; k < ParamTypes.size(); k++)
	{
		nameBuilder << (ParamTypes[k] ? ParamTypes[k]->TypeName() : TEXT("??"));
		if (k != ParamTypes.size() - 1)
		{
			nameBuilder << TEXT(", ");
		}
	} // end for k
	nameBuilder << TEXT(")");

	// assign attributes.
	pNew->pReturnType = pReturnType;
	pNew->ParamTypes = ParamTypes;
	pNew->szFunctionProto = nameBuilder.str();

	return pNew;
}

// get type name
std::wstring FSymFunctionType::TypeName() const
{
	return szFunctionProto;
}

// get format value
std::wstring FSymFunctionType::FormatValue(void *ValuePtr) const
{
	return TEXT("{ ..function.. }");
}

//////////////////////////////////////////////////////////////////////////

FSymTypedefType::FSymTypedefType()
	: pInnerType(NULL)
{
}

FSymTypedefType::~FSymTypedefType()
{
	if (pInnerType)
	{
		delete pInnerType; pInnerType = NULL;
	}
}

FSymTypedefType* FSymTypedefType::StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId)
{
	std::wstring DefName;

	TCHAR *pSymName = NULL;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_SYMNAME, &pSymName);
	DefName = pSymName;
	LocalFree(pSymName);

	DWORD InnerTypeId;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_TYPEID, &InnerTypeId);

	FSymTypedefType *pNew = new FSymTypedefType();
	if (pNew)
	{
		FSymTypeInfoHelper::CacheSymTypeInfo(InProcess, InModuleBase, TypeId, pNew);

		pNew->DefName = DefName;
		pNew->pInnerType = FSymTypeInfoHelper::BuildSymTypeInfo(InProcess, InModuleBase, InnerTypeId);
	}

	return pNew;
}

// get type name
std::wstring FSymTypedefType::TypeName() const
{
	return DefName;
}

// get format value
std::wstring FSymTypedefType::FormatValue(void *ValuePtr) const
{
	if (pInnerType)
	{
		return pInnerType->FormatValue(ValuePtr);
	}

	return TEXT("??");
}

//////////////////////////////////////////////////////////////////////////
FSymComplexType::FSymComplexType()
{
}

FSymComplexType::~FSymComplexType()
{
	for (size_t k=0; k<Members.size(); k++)
	{
		FMemberElement &Entry = Members[k];

		if (Entry.pTypeInfo)
		{
			delete Entry.pTypeInfo;
		}
	} // end for k

	Members.clear();
}

FSymComplexType* FSymComplexType::StaticCreate(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId)
{
	FSymComplexType *pNew = new FSymComplexType();
	if (!pNew)
	{
		return NULL;
	}
	FSymTypeInfoHelper::CacheSymTypeInfo(InProcess, InModuleBase, TypeId, pNew);

	std::wstring UserDefName;
	{
		TCHAR *pSymName = NULL;
		SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_SYMNAME, &pSymName);
		UserDefName = pSymName;
		LocalFree(pSymName);
	}

	std::vector<FMemberElement>	Members;
	{
		//获取成员数量
		DWORD MembersCount;
		SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_CHILDRENCOUNT, &MembersCount);

		//获取每个参数的类型名称
		BYTE* pBuffer = (BYTE*)malloc(sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG) * MembersCount);
		TI_FINDCHILDREN_PARAMS* pFindParams = (TI_FINDCHILDREN_PARAMS*)pBuffer;
		pFindParams->Count = MembersCount;
		pFindParams->Start = 0;

		SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_FINDCHILDREN, pFindParams);
		for (DWORD k = 0; k < MembersCount; k++)
		{
			const ULONG kChildId = pFindParams->ChildId[k];

			DWORD MemberTag;
			SymGetTypeInfo(InProcess, InModuleBase, kChildId, TI_GET_SYMTAG, &MemberTag);

			if (MemberTag != SymTagData && MemberTag != SymTagBaseClass)
			{
				continue;
			}

			std::wstring MemberName;

			TCHAR *pSymName = NULL;
			SymGetTypeInfo(InProcess, InModuleBase, kChildId, TI_GET_SYMNAME, &pSymName);
			MemberName = pSymName;
			LocalFree(pSymName);

			DWORD MemberTypeID;
			SymGetTypeInfo(InProcess, InModuleBase, kChildId, TI_GET_TYPEID, &MemberTypeID);

			DWORD MemberOffset;
			SymGetTypeInfo(InProcess, InModuleBase, kChildId, TI_GET_OFFSET, &MemberOffset);

			FSymTypeInfo *pMemberType = FSymTypeInfoHelper::BuildSymTypeInfo(InProcess, InModuleBase, MemberTypeID);
			
			FMemberElement MemberEntry;
			MemberEntry.Name = MemberName;
			MemberEntry.Offset = MemberOffset;
			MemberEntry.pTypeInfo = pMemberType;

			Members.push_back(MemberEntry);
		} // end for k
		free(pBuffer);
	}

	// assign attributes.
	pNew->UserTypeName = UserDefName;
	pNew->Members = Members;

	return pNew;
}

// get type name
std::wstring FSymComplexType::TypeName() const
{
	return UserTypeName;
}

// get format value
std::wstring FSymComplexType::FormatValue(void *ValuePtr) const
{
	std::wostringstream  valueBuilder;

	valueBuilder << TEXT("{ ");
	for (size_t k = 0; k < Members.size(); k++)
	{
		const FMemberElement &Entry = Members[k];
		if (Entry.pTypeInfo)
		{
			valueBuilder << Entry.Name << TEXT("(") << Entry.pTypeInfo->TypeName() << TEXT("): ") << Entry.pTypeInfo->FormatValue((BYTE *)ValuePtr + Entry.Offset)
				<< TEXT(", ");
		}
		else
		{
			valueBuilder << Entry.Name << TEXT(" ??, ");
		}
	} // end for k
	valueBuilder << TEXT(" }");

	return valueBuilder.str();
}

//////////////////////////////////////////////////////////////////////////
struct FSymTypeKey
{
	uint64_t	MouduleBase;
	uint32_t	TypeId;

	FSymTypeKey(uint64_t InModuleBase, uint32_t InTypeId)
		: MouduleBase(InModuleBase)
		, TypeId(InTypeId)
	{}

	bool operator ==(const FSymTypeKey &rhs) const
	{
		return (MouduleBase == rhs.MouduleBase) && (TypeId == rhs.TypeId);
	}

	bool operator <(const FSymTypeKey &rhs) const
	{
		return (MouduleBase < rhs.MouduleBase) || (TypeId < rhs.TypeId);
	}
};

static std::map<FSymTypeKey, FSymTypeInfo*> sSymTypeMap;

static void ClearSymTypeMap()
{
	for (std::map<FSymTypeKey, FSymTypeInfo*>::iterator Itr=sSymTypeMap.begin(); Itr!=sSymTypeMap.end(); ++Itr)
	{
		delete Itr->second;
	} // end for

	sSymTypeMap.clear();
}

void FSymTypeInfoHelper::Initialize()
{
	ClearSymTypeMap();
}

void FSymTypeInfoHelper::Uninitialize()
{
	ClearSymTypeMap();
}

void FSymTypeInfoHelper::CacheSymTypeInfo(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId, FSymTypeInfo *InTypeInfo)
{
	if (InTypeInfo)
	{
		const FSymTypeKey Key(InModuleBase, TypeId);

		sSymTypeMap.insert(std::make_pair(Key, InTypeInfo));
	}
}

FSymTypeInfo* FSymTypeInfoHelper::BuildSymTypeInfo(HANDLE InProcess, uint64_t InModuleBase, uint32_t TypeId)
{
	DWORD TypeTag;
	SymGetTypeInfo(InProcess, InModuleBase, TypeId, TI_GET_SYMTAG, &TypeTag);

	const FSymTypeKey Key(InModuleBase, TypeId);

	std::map<FSymTypeKey, FSymTypeInfo*>::iterator FindItr = sSymTypeMap.find(Key);
	if (FindItr != sSymTypeMap.end())
	{
		return FindItr->second;
	}

	FSymTypeInfo* pTypeInfo = NULL;
	switch (TypeTag)
	{
	case SymTagBaseType:
		pTypeInfo = FSymPrimitiveType::StaticCreate(InProcess, InModuleBase, TypeId);
		break;
	case SymTagPointerType:
		pTypeInfo = FSymPointerType::StaticCreate(InProcess, InModuleBase, TypeId);
		break;
	case SymTagEnum:
		pTypeInfo = FSymEnumType::StaticCreate(InProcess, InModuleBase, TypeId);
		break;
	case SymTagArrayType:
		pTypeInfo = FSymArrayType::StaticCreate(InProcess, InModuleBase, TypeId);
		break;
	case SymTagTypedef:
		pTypeInfo = FSymTypedefType::StaticCreate(InProcess, InModuleBase, TypeId);
		break;
	case SymTagFunctionType:
		pTypeInfo = FSymFunctionType::StaticCreate(InProcess, InModuleBase, TypeId);
		break;
	case SymTagUDT:
		pTypeInfo = FSymComplexType::StaticCreate(InProcess, InModuleBase, TypeId);
		break;
	default:
		pTypeInfo = FSymUnknownType::StaticCreate(InProcess, InModuleBase, TypeId);
	}

	return pTypeInfo;
}
