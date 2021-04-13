﻿#pragma once
#include <string>

namespace llang {
    
    namespace ast {
        enum class CONSTANT_TYPE;
    }

    /**
    * This enum contains all prmitives names
    **/
    enum class PRIMITIVE_TYPE
    {
        /* SPECIALS */
        VOID,
        BOOL,

        /* SIGNED */
        INT8,
        INT16,
        INT32,
        INT64,
        
        /* CHARACTERS */
        SCHAR = INT8,  // ASCII
        CHAR = INT64 + 1,   // UTF-8
        WCHAR,  // UTF-16
        UCHAR,  // UNICODE

        /* UNSIGNED */
        BYTE = CHAR,   // Alias for uint8
        UINT8 = BYTE,
        UINT16 = WCHAR,
        UINT32 = UCHAR,
        UINT64,
        
        /* FLOATING POINT */
        FLOAT32,
        FLOAT64       
    };

    /**
     * This is a holder for all primitives types.
     * Useful for checking if the type is primitive.
     **/
    namespace Primitives
    {
        bool Exists(const std::string &name);
        
        PRIMITIVE_TYPE Get(const std::string &name);
        std::string GetName(PRIMITIVE_TYPE primitive);
        std::string GetName(ast::CONSTANT_TYPE constType);
    };
}