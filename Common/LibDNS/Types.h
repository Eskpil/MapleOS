#pragma once

#include <AK/Types.h>

namespace DNS {

enum RecordType : u16 {
    TypeNone = 0,
    TypeA = 1,
    TypeNS = 2,
    TypeMD = 3,
    TypeMF = 4,
    TypeCNAME = 5,
    TypeSOA = 6,
    TypeMB = 7,
    TypeMG = 8,
    TypeMR = 9,
    TypeNULL = 10,
    TypePTR = 12,
    TypeHINFO = 13,
    TypeMINFO = 14,
    TypeMX = 15,
    TypeTXT = 16,
    TypeRP = 17,
    TypeAFSDB = 18,
    TypeX25 = 19,
    TypeISDN = 20,
    TypeRT = 21,
    TypeNSAPPTR = 23,
    TypeSIG = 24,
    TypeKEY = 25,
    TypePX = 26,
    TypeGPOS = 27,
    TypeAAAA = 28,
    TypeLOC = 29,
    TypeNXT = 30,
    TypeEID = 31,
    TypeNIMLOC = 32,
    TypeSRV = 33,
    TypeATMA = 34,
    TypeNAPTR = 35,
    TypeKX = 36,
    TypeCERT = 37,
    TypeDNAME = 39,
    TypeOPT = 41, // EDNS
    TypeAPL = 42,
    TypeDS = 43,
    TypeSSHFP = 44,
    TypeRRSIG = 46,
    TypeNSEC = 47,
    TypeDNSKEY = 48,
    TypeDHCID = 49,
    TypeNSEC3 = 50,
    TypeNSEC3PARAM = 51,
    TypeTLSA = 52,
    TypeSMIMEA = 53,
    TypeHIP = 55,
    TypeNINFO = 56,
    TypeRKEY = 57,
    TypeTALINK = 58,
    TypeCDS = 59,
    TypeCDNSKEY = 60,
    TypeOPENPGPKEY = 61,
    TypeCSYNC = 62,
    TypeZONEMD = 63,
    TypeSVCB = 64,
    TypeHTTPS = 65,
    TypeSPF = 99,
    TypeUINFO = 100,
    TypeUID = 101,
    TypeGID = 102,
    TypeUNSPEC = 103,
    TypeNID = 104,
    TypeL32 = 105,
    TypeL64 = 106,
    TypeLP = 107,
    TypeEUI48 = 108,
    TypeEUI64 = 109,
    TypeURI = 256,
    TypeCAA = 257,
    TypeAVC = 258,

    TypeTKEY = 249,
    TypeTSIG = 250,

    // valid Question.Qtype only
    TypeIXFR = 251,
    TypeAXFR = 252,
    TypeMAILB = 253,
    TypeMAILA = 254,
    TypeANY = 255,

    TypeTA = 32768,
    TypeDLV = 32769,
    TypeReserved = 65535,
};

enum QuestionType : u16 {
    ClassINET = 1,
    ClassCSNET = 2,
    ClassCHAOS = 3,
    ClassHESIOD = 4,
    ClassNONE = 254,
    ClassANY = 255,
};

}