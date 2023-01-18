// Copyright (c) 2017-2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UKKEY_SIMPLIFIEDMNS_H
#define UKKEY_SIMPLIFIEDMNS_H

#include "bls/bls.h"
#include "merkleblock.h"
#include "netaddress.h"
#include "pubkey.h"
#include "serialize.h"

class UniValue;
class CDeterministicMNList;
class CDeterministicMN;

class CSimplifiedMNListEntry
{
public:
    uint256 proRegTxHash;
    uint256 confirmedHash;
    CService service;
    CBLSPublicKey pubKeyOperator;
    CKeyID keyIDVoting;
    bool isValid;

public:
    CSimplifiedMNListEntry() {}
    CSimplifiedMNListEntry(const CDeterministicMN& dmn);

    bool operator==(const CSimplifiedMNListEntry& rhs) const
    {
        return proRegTxHash == rhs.proRegTxHash &&
               confirmedHash == rhs.confirmedHash &&
               service == rhs.service &&
               pubKeyOperator == rhs.pubKeyOperator &&
               keyIDVoting == rhs.keyIDVoting &&
               isValid == rhs.isValid;
    }

    bool operator!=(const CSimplifiedMNListEntry& rhs) const
    {
        return !(rhs == *this);
    }

public:
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(proRegTxHash);
        READWRITE(confirmedHash);
        READWRITE(service);
        READWRITE(pubKeyOperator);
        READWRITE(keyIDVoting);
        READWRITE(isValid);
    }

public:
    uint256 CalcHash() const;

    std::string ToString() const;
    void ToJson(UniValue& obj) const;
};

class CSimplifiedMNList
{
public:
    std::vector<CSimplifiedMNListEntry> mnList;

public:
    CSimplifiedMNList() {}
    CSimplifiedMNList(const std::vector<CSimplifiedMNListEntry>& smlEntries);
    CSimplifiedMNList(const CDeterministicMNList& dmnList);

    uint256 CalcMerkleRoot(bool* pmutated = NULL) const;
};

/// P2P messages

class CGetSimplifiedMNListDiff
{
public:
    uint256 baseBlockHash;
    uint256 blockHash;

public:
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(baseBlockHash);
        READWRITE(blockHash);
    }
};

class CSimplifiedMNListDiff
{
public:
    uint256 baseBlockHash;
    uint256 blockHash;
    CPartialMerkleTree cbTxMerkleTree;
    CTransactionRef cbTx;
    std::vector<uint256> deletedMNs;
    std::vector<CSimplifiedMNListEntry> mnList;

public:
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(baseBlockHash);
        READWRITE(blockHash);
        READWRITE(cbTxMerkleTree);
        READWRITE(cbTx);
        READWRITE(deletedMNs);
        READWRITE(mnList);
    }

public:
    void ToJson(UniValue& obj) const;
};

bool BuildSimplifiedMNListDiff(const uint256& baseBlockHash, const uint256& blockHash, CSimplifiedMNListDiff& mnListDiffRet, std::string& errorRet);

#endif //UKKEY_SIMPLIFIEDMNS_H
