// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"

#include "assert.h"
#include "core.h"
#include "protocol.h"
#include "util.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

//
// Main network
//

unsigned int pnSeed[] =
{
    0x12345678
};

class CMainParams : public CChainParams {
public:
    CMainParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x05;
        pchMessageStart[1] = 0x06;
        pchMessageStart[2] = 0x07;
        pchMessageStart[3] = 0x08;
        vAlertPubKey = ParseHex("04cd2f7f7ecb4ec908b9f6814ebe8dbe85c61b76069f5da9f5334c9a76fce08e335246d9c216500ac50208bc702820889e91f2c3ab543210769272198352478d26");
        nDefaultPort = 51400;
        nRPCPort = 51401;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 20);
        nSubsidyHalvingInterval = 140000; 

        // Genesis block
        const char* pszTimestamp = "Oh, and Emil...Ever Forward.";
        CTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 16 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("04becedf6ebadd4596964d890f677f8d2e74fdcc313c6416434384a66d6d8758d1c92de272dc6713e4a81d98841dfdfdc95e204ba915447d2fe9313435c78af3e8") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1717110000;
        genesis.nBits    = 0x1e0fffff;
        genesis.nNonce   = 3219587;

        hashGenesisBlock = genesis.GetHash();

        assert(hashGenesisBlock == uint256("0x000006d2f89bed88605c67665f7154e9520aecfe68a7deacf176672b4248c9df"));
        assert(genesis.hashMerkleRoot == uint256("0x185e7333b9f1825cde09fac260b808fc039ca1052280411fec4cd04ffed932c0"));

        vSeeds.push_back(CDNSSeedData("bits.silentpatriot.com", "bits.silentpatriot.com"));
        vSeeds.push_back(CDNSSeedData("216.238.81.202", "216.238.81.202"));
        vSeeds.push_back(CDNSSeedData("community seed 01", "64.176.193.63"));
        vSeeds.push_back(CDNSSeedData("community seed 02", "149.28.233.244"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,18);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,18);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,28 + 128);            
        
        for (unsigned int i = 0; i < ARRAYLEN(pnSeed); i++)
        {
            const int64_t nOneWeek = 7*24*60*60;
            struct in_addr ip;
            memcpy(&ip, &pnSeed[i], sizeof(ip));
            CAddress addr(CService(ip, GetDefaultPort()));
            addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
            vFixedSeeds.push_back(addr);
        }
    }

    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    CBlock genesis;
    vector<CAddress> vFixedSeeds;
};
static CMainParams mainParams;


//
// Testnet (v3)
//
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        pchMessageStart[0] = 0x08;
        pchMessageStart[1] = 0x07;
        pchMessageStart[2] = 0x06;
        pchMessageStart[3] = 0x05;

        vAlertPubKey = ParseHex("049cfe36745551756a4cb3c7479adfd65cc372cf7e80072fc90e3578a2aa787f29b1d2903eb7edb953213e2bf03810374f4a478e0d6c6af924dc0ba16f26903840");

        nDefaultPort = 14000;
        nRPCPort = 14001;
        strDataDir = "testnet";

        genesis.nTime = 1609959960;
        genesis.nNonce = 1644211;

        hashGenesisBlock = genesis.GetHash();   
                 
        assert(hashGenesisBlock == uint256("0x00000dae3b9b9f559a4a5d0b0fe71c81c5cff2268b600789ee5e04e8a5ed1660"));

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("bits.silentpatriot.com", "bits.silentpatriot.com"));
        vSeeds.push_back(CDNSSeedData("216.238.81.202", "216.238.81.202"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,43);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,43);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,88 + 128);

    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
};
static CTestNetParams testNetParams;


//
// Regression test
//
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        pchMessageStart[0] = 0x01;
        pchMessageStart[1] = 0x02;
        pchMessageStart[2] = 0x03;
        pchMessageStart[3] = 0x04;
        nSubsidyHalvingInterval = 150;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 20);
        genesis.nTime = 1296688602;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 1618224;
        nDefaultPort = 18444;
        strDataDir = "regtest";

        hashGenesisBlock = genesis.GetHash();        
        
        vSeeds.clear();  
    }

    virtual bool RequireRPCPassword() const { return false; }
    virtual Network NetworkID() const { return CChainParams::REGTEST; }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = &mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

void SelectParams(CChainParams::Network network) {
    switch (network) {
        case CChainParams::MAIN:
            pCurrentParams = &mainParams;
            break;
        case CChainParams::TESTNET:
            pCurrentParams = &testNetParams;
            break;
        case CChainParams::REGTEST:
            pCurrentParams = &regTestParams;
            break;
        default:
            assert(false && "Unimplemented network");
            return;
    }
}

bool SelectParamsFromCommandLine() {
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet && fRegTest) {
        return false;
    }

    if (fRegTest) {
        SelectParams(CChainParams::REGTEST);
    } else if (fTestNet) {
        SelectParams(CChainParams::TESTNET);
    } else {
        SelectParams(CChainParams::MAIN);
    }
    return true;
}
