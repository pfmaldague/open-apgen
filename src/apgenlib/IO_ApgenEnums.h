#ifndef APGENTypes_h
#define APGENTypes_h


typedef enum 
{
  APGENExpansionActive = 1,
  APGENExpansionDecomposed = 2,
  APGENExpansionAbstracted = 3
} ExpansionState;


typedef enum 
{
  APGENDecompNoDetail = 0,
  APGENDecompDecomposition = 1,
  APGENDecompResolution = 2

} DecompositionType;


typedef enum 
{
  APGENValueUnInitialized = 0,
  APGENValueInt = 1,
  APGENValueDouble = 2,
  APGENValueString = 3,
  APGENValueTime = 4,
  APGENValueArraySet = 5,
  APGENValueArrayAppend = 6,
  APGENValueBool = 7,
  APGENValueInstance = 8,
  APGENValueDuration = 9

}  ValueType;


typedef enum 
{
  APGENArrayIndexNumeric = 0,
  APGENArrayIndexString = 1
}  ArrayIndexType;


typedef enum
{
  APGENOptionsFileBrowse = 0,
  APGENOptionsOther = 1
}  APGENOptionsType;

#endif
