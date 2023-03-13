// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2023 The Volkshash Core Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include "arith_uint256.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateDevNetGenesisBlock(const uint256 &prevBlockHash, const std::string& devNetName, uint32_t nTime, uint32_t nNonce, uint32_t nBits, const CAmount& genesisReward)
{
    assert(!devNetName.empty());

    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    // put height (BIP34) and devnet name into coinbase
    txNew.vin[0].scriptSig = CScript() << 1 << std::vector<unsigned char>(devNetName.begin(), devNetName.end());
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = CScript() << OP_RETURN;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = 4;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock = prevBlockHash;
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}


static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "13Dec2022 FTX SBF ARRESTED R R";
    const CScript genesisOutputScript = CScript() << ParseHex("0485a729f67928d5fa19e8079687e50f0fd7ecf9a75d24db010ba72a493b33aefee507465c380a05c62578ccec60686dff8b5fb1f369dedf6d702c504353f7ca06") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

static CBlock FindDevNetGenesisBlock(const Consensus::Params& params, const CBlock &prevBlock, const CAmount& reward)
{
    std::string devNetName = GetDevNetName();
    assert(!devNetName.empty());

    CBlock block = CreateDevNetGenesisBlock(prevBlock.GetHash(), devNetName.c_str(), prevBlock.nTime + 1, 0, prevBlock.nBits, reward);

    arith_uint256 bnTarget;
    bnTarget.SetCompact(block.nBits);

    for (uint32_t nNonce = 0; nNonce < UINT32_MAX; nNonce++) {
        block.nNonce = nNonce;

        uint256 hash = block.GetHash();
        if (UintToArith256(hash) <= bnTarget)
            return block;
    }

    // This is very unlikely to happen as we start the devnet with a very low difficulty. In many cases even the first
    // iteration of the above loop will give a result already
    error("FindDevNetGenesisBlock: could not find devnet genesis block for %s", devNetName);
    assert(false);
}

// this one is for testing only
static Consensus::LLMQParams llmq10_60 = {
        .type = Consensus::LLMQ_10_60,
        .name = "llmq_10",
        .size = 10,
        .minSize = 6,
        .threshold = 6,

        .dkgInterval = 24, // one DKG per hour
        .dkgPhaseBlocks = 2,
        .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 18,
};

static Consensus::LLMQParams llmq50_60 = {
        .type = Consensus::LLMQ_50_60,
        .name = "llmq_50_60",
        .size = 50,
        .minSize = 40,
        .threshold = 30,

        .dkgInterval = 24, // one DKG per hour
        .dkgPhaseBlocks = 2,
        .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 18,
};

static Consensus::LLMQParams llmq400_60 = {
        .type = Consensus::LLMQ_400_60,
        .name = "llmq_400_51",
        .size = 400,
        .minSize = 300,
        .threshold = 240,

        .dkgInterval = 24 * 12, // one DKG every 12 hours
        .dkgPhaseBlocks = 4,
        .dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 28,
};

// Used for deployment and min-proto-version signalling, so it needs a higher threshold
static Consensus::LLMQParams llmq400_85 = {
        .type = Consensus::LLMQ_400_85,
        .name = "llmq_400_85",
        .size = 400,
        .minSize = 350,
        .threshold = 340,

        .dkgInterval = 24 * 24, // one DKG every 24 hours
        .dkgPhaseBlocks = 4,
        .dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 48, // give it a larger mining window to make sure it is mined
};

//Transaction Records of Genesisblocks 

   //main ---
  // nonce: 139786
  // time: 1670440473
  // MAIN NET hash: 0000035502f6f464645ff5caa344484f01089f2020712fbd76b79a82ed92d91f
//merklehash: 7a49746bd2b2105991efd1529ae3f1ebf9e9eb392f163eec5bf69bf67673f669

//test ---
 // nonce: 15719
  // time: 1670440400
 //  TEST NET hash: 000009dc62e5bc38bae3e5fa53b5e667c06a2066d32c12343d76bc540772b732
// merklehash: 7a49746bd2b2105991efd1529ae3f1ebf9e9eb392f163eec5bf69bf67673f669
//regtest ---
  //nonce: 0
   //time: 1670440200
   //REG NET hash: f824752aa49a98228a86b65acd7b0c72c7e86d9a94107d158825a7c243c33083
   //merklehash: 7a49746bd2b2105991efd1529ae3f1ebf9e9eb392f163eec5bf69bf67673f669


   


class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyFiftheningInterval = 87600;                 	// TODO :  Enabled 0.5 Years a reduction of 20% occurs Fifthening
        consensus.nMasternodePaymentsStartBlock = 1;               	// Enabled Not True as long as it's less then nMasternodePaymentsIncreaseBlock
        consensus.nMasternodePaymentsIncreaseBlock = 2100000000;        // Disabled Masternode Payments Increase Block
        consensus.nMasternodePaymentsIncreasePeriod = 2100000000;       // Disabled Masternode Payments Increase Period
        consensus.nInstantSendConfirmationsRequired = 6;                // Enabled Instant Send 
        consensus.nInstantSendKeepLock = 24;                            // Enabled Instant Send Keep
        consensus.nBudgetPaymentsStartBlock = 2100000000;               // Disabled Budget Payments Start Block
        consensus.nBudgetPaymentsCycleBlocks = 2100000000;              // Disabled Budget Payments Cycle Blocks
        consensus.nBudgetPaymentsWindowBlocks = 2100000000;             // Disabled Budget Payments Window Blocks
        consensus.nSuperblockStartBlock = 2100000000;                   // Disabled Super block Start Block
        consensus.nSuperblockStartHash = uint256();                     // Disabled Super block Start Block Hash
        consensus.nSuperblockCycle = 2100000000;                        // Disabled SuperblockCycle
        consensus.nGovernanceMinQuorum = 10;                            // Quorum TODO future 
        consensus.nGovernanceFilterElements = 20000;                    // Governance Filter Elements
        consensus.nMasternodeMinimumConfirmations = 15;                 // Enabled Masternode Minimum Confirmations
        consensus.BIP34Height = 34;                                     // BIP 34                   Start Block 34
        consensus.BIP34Hash = uint256();                                // TODO add BIP 34 HASH
        consensus.BIP65Height = 65;                                     // BIP 65                   Start Block 65
        consensus.BIP66Height = 66;                                     // BIP 66                   Start Block 66
        consensus.DIP0001Height = 1;                                    // DIP0001                  Start Block 1
        consensus.powLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // Pow Limit  
        consensus.nPowTargetTimespan = 3 * 60 * 60;                     // Volkshash: Every 60 Blocks 
        consensus.nPowTargetSpacing = 3 * 60;                           // Volkshash: 3 Minute Block Time 
        consensus.fPowAllowMinDifficultyBlocks = false;                 // Disabled Pow Allow Min Difficulty Blocks

        consensus.fPowNoRetargeting = false;                            // Disabled Pow No Retarget
        consensus.nPowKGWHeight = 1;                                    // Kimoto's Gravity Well Legacy Starts and Ends on Block 0
        consensus.nPowDGWHeight = 1;                                    // Dark Gravity Wave Start Starts on Block 0
        
        consensus.nRuleChangeActivationThreshold = 1916;                // Ancient Deploy Rules: Keeping As Ref:   95% of 2016
        consensus.nMinerConfirmationWindow = 2016;                      // Ancient Deploy Rules: nPowTargetTimespan / nPowTargetSpacing
                
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601;        // Ancient Deploy Rules: January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999;          // Ancient Deploy Rules: December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1486252800;              // Ancient Deploy Rules: Feb 5th, 2017 
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1517788800;                // Ancient Deploy Rules: Feb 5th, 2018

        // Deployment of DIP0001
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1508025600;          // Ancient Deploy Rules: Oct 15th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1539561600;            // Ancient Deploy Rules: Oct 15th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 4032;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 3226;                // Ancient Deploy Rules: 80% of 4032

        // Deployment of BIP147
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1524477600;           // Ancient Deploy Rules: Apr 23th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1556013600;             // Ancient Deploy Rules: Apr 23th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 4032;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 3226;                 // Ancient Deploy Rules: 80% of 4032

        // Deployment of DIP0003
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 1546300800;          // Ancient Deploy Rules: Jan 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 1577836800;            // Ancient Deploy Rules: Jan 1st, 2020
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 4032;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThreshold = 3226;                // Ancient Deploy Rules: 80% of 4032

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid. Same as genesis as there was no previous blocks 
        consensus.defaultAssumeValid = uint256S("0x0000035502f6f464645ff5caa344484f01089f2020712fbd76b79a82ed92d91f");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xde;                                      //peerMagic Mainet      
        pchMessageStart[1] = 0xad;                                      //peerMagic Mainet
        pchMessageStart[2] = 0xf1;                                      //peerMagic Mainet
        pchMessageStart[3] = 0xb1;                                      //peerMagic Mainet
        
                                                                        // Used By NOMP
                                                                        // peerMagic Mainet Combined :deadf1b1  
        
        vAlertPubKey = ParseHex("04495dd254658385071f8fa34a8ed86166cf60cafeb52f9535029bb7a3eb6dd159811d7911ca2b25a39b4d4b30a2e502fa6491ecfed381dd4f03cd50b4617fbd92");
        nDefaultPort = 17374;           
        nPruneAfterHeight = 100000;

        // Creation Of The Genesis Rules
        genesis = CreateGenesisBlock(1670440473, 139786, 0x1f00ffff, 1, 50 * COIN);

        // Asserting Block Hash Of The Genesis 
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0000035502f6f464645ff5caa344484f01089f2020712fbd76b79a82ed92d91f"));
        assert(genesis.hashMerkleRoot == uint256S("0x7a49746bd2b2105991efd1529ae3f1ebf9e9eb392f163eec5bf69bf67673f669"));

        // Seeds and Nodes for Volkshash 
        vSeeds.push_back(CDNSSeedData("pool.volkshash.org", "explorer.volkshash.org"));
	//TODO  Seeds Temp Solution for Nodes early start . Seeder to be launched a bit later 
        vSeeds.push_back(CDNSSeedData("102.219.85.134", "102.219.85.87"));


        // VolkshashMainent addresses start with 'V'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,70);
        // Volkshash script addresses start with '7'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,122);
        // Volkshash private keys start with '7' or 'X'
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,112);
        
        
        // Volkshash BIP32 pubkeys start with 'xpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x1A)(0xA8)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        // Volkshash BIP32 prvkeys start with 'xprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x14)(0x88)(0x1D)(0xE1).convert_to_container<std::vector<unsigned char> >();

        // Volkshash BIP44 coin type is '20'
        nExtCoinType = 20;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        // long living quorum params
        consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
        consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
        consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fRequireRoutableExternalIP = true;
        fMineBlocksOnDemand = false;
        fAllowMultipleAddressesFromGroup = false;
        fAllowMultiplePorts = false;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour

        vSporkAddresses = {"VHashBurnGXXXXXXXXXXXXXXXXXXUVXsg2"};
        nMinSporkKeys = 1;
        fBIP9CheckMasternodesUpgraded = true;
        consensus.fLLMQAllowDummyCommitments = false;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (     0, uint256S("0x0000035502f6f464645ff5caa344484f01089f2020712fbd76b79a82ed92d91f"))
        };

        chainTxData = ChainTxData{
            1670440473, // * UNIX timestamp of last known number of transactions
            0,          // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            0           // * estimated number of transactions per second after that timestamp
        };

                                        // Developement Fee Reward at 3 %
        strFounderAddress = "VTQm6cWqEmc85168N2Ag8PtdC3emvcEyaL";
        dFounderFee = 0.03;             //Active from Block 100

    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyFiftheningInterval = 2100000000;
        consensus.nMasternodePaymentsStartBlock = 1000; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
        consensus.nMasternodePaymentsIncreaseBlock = 2100000000;
        consensus.nMasternodePaymentsIncreasePeriod = 2100000000;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 2100000000;         // Disabled Budget Payments Start Block
        consensus.nBudgetPaymentsCycleBlocks = 2100000000;        // Disabled Budget Payments Cycle Blocks
        consensus.nBudgetPaymentsWindowBlocks = 2100000000;       // Disabled Budget Payments Window Blocks
        consensus.nSuperblockStartBlock = 2100000000;             // Disabled Super block Start Block
        consensus.nSuperblockStartHash = uint256();
        consensus.nSuperblockCycle = 2100000000;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 34;
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 65;
        consensus.BIP66Height = 66;
        consensus.DIP0001Height = 1;
        consensus.powLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
        consensus.nPowTargetTimespan = 1 * 60 * 60; // Volkshash: 1 hour
        consensus.nPowTargetSpacing = 1 * 60;       // Volkshash: 1 minutes
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nPowKGWHeight = 0; // nPowKGWHeight >= nPowDGWHeight means "no KGW"
        consensus.nPowDGWHeight = 0;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1544655600; // Dec 13th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1576191600; // Dec 13th, 2019

        // Deployment of DIP0001
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1544655600; // Dec 13th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1576191600; // Dec 13th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 50; // 50% of 100

        // Deployment of BIP147
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1544655600; // Dec 13th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1576191600; // Dec 13th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 50; // 50% of 100

        // Deployment of DIP0003
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 1544655600; // Dec 13th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 1576191600; // Dec 13th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThreshold = 50; // 50% of 100

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x000009dc62e5bc38bae3e5fa53b5e667c06a2066d32c12343d76bc540772b732");

        pchMessageStart[0] = 0xce; // peerMagic 
        pchMessageStart[1] = 0xa2; // peerMagic 
        pchMessageStart[2] = 0xca; // peerMagic 
        pchMessageStart[3] = 0xab; // peerMagic 
                                                                        // Used By NOMP / POOLS
                                                                        // peerMagic Testnet Combined :cea2caab  


        vAlertPubKey = ParseHex("04835db5e87fd67aa638e06d9de344a6bba384e1ae85473ca6515dacd183a6bce2e04e6e0e5619a343ee48ec318367fabf4cc57fc8ea36adccd20bcdd0c08c9a2f");
        nDefaultPort = 17374;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1670440400, 15719, 0x1f00ffff, 1, 50 * COIN);


        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000009dc62e5bc38bae3e5fa53b5e667c06a2066d32c12343d76bc540772b732"));
        assert(genesis.hashMerkleRoot == uint256S("0x7a49746bd2b2105991efd1529ae3f1ebf9e9eb392f163eec5bf69bf67673f669"));

        vFixedSeeds.clear();
        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        // vSeeds.push_back(CDNSSeedData("pool.volkshash.org", "dnsseed.pool.volkshash.org"));


        // VolkshashTestNet addresses start with 'v'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,132);
        // Testnet Volkshash script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,124);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,114);
        // Testnet Volkshash BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x02)(0x31)(0x83)(0xEF).convert_to_container<std::vector<unsigned char> >();
        // Testnet Volkshash BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x03)(0x32)(0x82)(0x91).convert_to_container<std::vector<unsigned char> >();

        // Testnet Volkshash BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        // long living quorum params
        consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
        consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
        consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fRequireRoutableExternalIP = true;
        fMineBlocksOnDemand = false;
        fAllowMultipleAddressesFromGroup = false;
        fAllowMultiplePorts = false;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        vSporkAddresses = {"vHashBurnGXXXXXXXXXXXXXXXXXXT8HGTG"};
        nMinSporkKeys = 1;
        fBIP9CheckMasternodesUpgraded = true;
        consensus.fLLMQAllowDummyCommitments = true;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (     0, uint256S("0x000009dc62e5bc38bae3e5fa53b5e667c06a2066d32c12343d76bc540772b732"))
        };

        chainTxData = ChainTxData{
            1670440400, // * UNIX timestamp of last known number of transactions
            0,                 // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            0          // * estimated number of transactions per second after that timestamp
        };

        // founder address & fee
        strFounderAddress = "vHashBurnGXXXXXXXXXXXXXXXXXXT8HGTG";

    }
};
static CTestNetParams testNetParams;

/**
 * Devnet
 */
class CDevNetParams : public CChainParams {
public:
    CDevNetParams() {
        strNetworkID = "dev";
        consensus.nSubsidyFiftheningInterval = 2100000000; // Custom Reward Schedule 
        consensus.nMasternodePaymentsStartBlock = 2100000000; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
        consensus.nMasternodePaymentsIncreaseBlock = 2100000000;
        consensus.nMasternodePaymentsIncreasePeriod = 2100000000;
        consensus.nInstantSendConfirmationsRequired = 2100000000;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 2100000000;
        consensus.nBudgetPaymentsCycleBlocks = 2100000000;
        consensus.nBudgetPaymentsWindowBlocks = 2100000000;
        consensus.nSuperblockStartBlock = 2100000000; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPeymentsStartBlock
        consensus.nSuperblockStartHash = uint256(); // do not check this on devnet
        consensus.nSuperblockCycle = 2100000000; // Superblocks can be issued hourly on devnet
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 1; // BIP34 activated immediately on devnet
        consensus.BIP65Height = 1; // BIP65 activated immediately on devnet
        consensus.BIP66Height = 1; // BIP66 activated immediately on devnet
        consensus.DIP0001Height = 2; // DIP0001 activated immediately on devnet
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Volkshash: 1 day
        consensus.nPowTargetSpacing = 2.5 * 60; // Volkshash: 2.5 minutes
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nPowKGWHeight = 4001; // nPowKGWHeight >= nPowDGWHeight means "no KGW"
        consensus.nPowDGWHeight = 4001;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1506556800; // September 28th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1538092800; // September 28th, 2018

        // Deployment of DIP0001
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1505692800; // Sep 18th, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1537228800; // Sep 18th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 50; // 50% of 100

        // Deployment of BIP147
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1517792400; // Feb 5th, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1549328400; // Feb 5th, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 50; // 50% of 100

        // Deployment of DIP0003
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 1535752800; // Sep 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 1567288800; // Sep 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThreshold = 50; // 50% of 100

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

        pchMessageStart[0] = 0xe2; // peerMagic
        pchMessageStart[1] = 0xca; // peerMagic
        pchMessageStart[2] = 0xff; // peerMagic
        pchMessageStart[3] = 0xce; // peerMagic

									                                    // Used By NOMP / POOLS
                                                                        // peerMagic DevNet Combined :e2caffce  
        vAlertPubKey = ParseHex("04835db5e87fd67aa638e06d9de344a6bba384e1ae85473ca6515dacd183a6bce2e04e6e0e5619a343ee48ec318367fabf4cc57fc8ea36adccd20bcdd0c08c9a2f");
        nDefaultPort = 27374;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1670440350, 1, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0"));
        assert(genesis.hashMerkleRoot == uint256S("0x0"));

        devnetGenesis = FindDevNetGenesisBlock(consensus, genesis, 50 * COIN);
        consensus.hashDevnetGenesisBlock = devnetGenesis.GetHash();

        vFixedSeeds.clear();
        vSeeds.clear();
        //vSeeds.push_back(CDNSSeedData("volkshashevo.org",  "devnet-seed.volkshashevo.org"));

        // VolkshashTestNet addresses start with 'r'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,130);
        // Testnet Volkshash script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet Volkshash BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        // Testnet Volkshash BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Testnet Volkshash BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        // long living quorum params
        consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
        consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
        consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fAllowMultipleAddressesFromGroup = true;
        fAllowMultiplePorts = true;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        vSporkAddresses = {"uSpTstvGf1FcyfbnkFMRhXs1ZiLGzacQQw"};
        nMinSporkKeys = 1;
        // devnets are started with no blocks and no MN, so we can't check for upgraded MN (as there are none)
        fBIP9CheckMasternodesUpgraded = false;
        consensus.fLLMQAllowDummyCommitments = true;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (     0, uint256S("0x0"))
        };

        chainTxData = ChainTxData{
            devnetGenesis.GetBlockTime(), // * UNIX timestamp of devnet genesis block
            2,                            // * we only have 2 coinbase transactions when a devnet is started up
            0                            // * estimated number of transactions per second
        };
    }

    void UpdateSubsidyAndDiffParams(int nMinimumDifficultyBlocks, int nHighSubsidyBlocks, int nHighSubsidyFactor)
    {
        consensus.nMinimumDifficultyBlocks = nMinimumDifficultyBlocks;
        consensus.nHighSubsidyBlocks = nHighSubsidyBlocks;
        consensus.nHighSubsidyFactor = nHighSubsidyFactor;
    }
};
static CDevNetParams *devNetParams;


/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyFiftheningInterval = 150;
        consensus.nMasternodePaymentsStartBlock = 1000;
        consensus.nMasternodePaymentsIncreaseBlock = 2100000000;
        consensus.nMasternodePaymentsIncreasePeriod = 2100000000;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 2100000000;         // Disabled
        consensus.nBudgetPaymentsCycleBlocks = 2100000000;        // Disabled
        consensus.nBudgetPaymentsWindowBlocks = 2100000000;       // Disabled
        consensus.nSuperblockStartBlock = 2100000000;             // Disabled
        consensus.nSuperblockStartHash = uint256();
        consensus.nSuperblockCycle = 2100000000;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 100;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.DIP0001Height = 0;
        consensus.powLimit = uint256S("0xffff000000000000000000000000000000000000000000000000000000000000");
        consensus.nPowTargetTimespan = 0.5 * 60 * 60; // Volkshash: 0.5 day
        consensus.nPowTargetSpacing = 1; // Volkshash: 1 sec
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nPowKGWHeight = 0; // same as mainnet
        consensus.nPowDGWHeight = 0; // same as mainnet
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0xf824752aa49a98228a86b65acd7b0c72c7e86d9a94107d158825a7c243c33083");

        pchMessageStart[0] = 0xfc;
        pchMessageStart[1] = 0xc1;
        pchMessageStart[2] = 0xb7;
        pchMessageStart[3] = 0xdc;
        nDefaultPort = 37374;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1670440200, 0, 0x2100ffff, 1, 50 * COIN);

        consensus.hashGenesisBlock = genesis.GetHash();
      
// ========================================================================================

// ========================================================================================
// Create Genesis Block KEEPING THIS FOR REF
// ========================================================================================
//if (false) {
//    bool fNegative;
//    bool fOverflow;
//
//    std::cout << "Begin calculating " << strNetworkID << " Genesis Block:\n";
//    arith_uint256 hashTarget = arith_uint256().SetCompact(genesis.nBits, &fNegative, &fOverflow);
//    std::cout << "fNegative: "<< fNegative << "\n";
//    std::cout << "fOverflow: "<< fOverflow << "\n";
//    uint256 hash;
//    genesis.nNonce = 0;
//    std::cout << " target: " << hashTarget.ToString().c_str() << "\n";
//    while (UintToArith256(genesis.GetHash()) > hashTarget) {
//        ++genesis.nNonce;
//        if (genesis.nNonce == 0) {
//            std::cout << std::string("NONCE WRAPPED, incrementing time:\n");
//            ++genesis.nTime;
//        }
//        if (genesis.nNonce % 100 == 0) {
//            std::cout << "[" << strNetworkID << "] nonce: " << genesis.nNonce << " hash: " << genesis.GetHash().ToString().c_str() << "\n";
//        }
//    }
//    std::cout << strNetworkID << " ---\n";
//    std::cout << "  nonce: " << genesis.nNonce <<  "\n";
//    std::cout << "   time: " << genesis.nTime << "\n";
//    std::cout << "   hash: " << genesis.GetHash().ToString().c_str() << "\n";
//    std::cout << "   merklehash: "  << genesis.hashMerkleRoot.ToString().c_str() << "\n";
//    std::cout << "REG NET Finished calculating " << strNetworkID << " Genesis Block:\n";
//}
// ========================================================================================

        assert(consensus.hashGenesisBlock == uint256S("0xf824752aa49a98228a86b65acd7b0c72c7e86d9a94107d158825a7c243c33083"));
        assert(genesis.hashMerkleRoot == uint256S("0x7a49746bd2b2105991efd1529ae3f1ebf9e9eb392f163eec5bf69bf67673f669"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fRequireRoutableExternalIP = false;
        fMineBlocksOnDemand = true;
        fAllowMultipleAddressesFromGroup = true;
        fAllowMultiplePorts = true;

        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        // privKey: 92cxgcNf2hNjnAyarcqFhpGbHon6e13dCHD6UPCyYCnsMvSHo4X
        vSporkAddresses = {"uSpReg6V2jz1mfQQBpAQ4xhHPiPLYozkKh"};
        nMinSporkKeys = 1;
        // regtest usually has no masternodes in most tests, so don't check for upgraged MNs
        fBIP9CheckMasternodesUpgraded = false;
        consensus.fLLMQAllowDummyCommitments = true;

        // founder address & fee
        strFounderAddress = "ufoReg11EUpkCNvKLgQpYnH7FtesEwTcSi";
        dFounderFee = 0.01;     // 1%           Not Used

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            (     0, uint256S("0xf824752aa49a98228a86b65acd7b0c72c7e86d9a94107d158825a7c243c33083"))
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        // Regtest Volkshash addresses start with 'u'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,130);
        // Regtest Volkshash script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Regtest private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Regtest Volkshash BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        // Regtest Volkshash BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Regtest Volkshash BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        // long living quorum params
        consensus.llmqs[Consensus::LLMQ_10_60] = llmq10_60;
        consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
    }

    void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }

    void UpdateBudgetParameters(int nMasternodePaymentsStartBlock, int nBudgetPaymentsStartBlock, int nSuperblockStartBlock)
    {
        consensus.nMasternodePaymentsStartBlock = nMasternodePaymentsStartBlock;
        consensus.nBudgetPaymentsStartBlock = nBudgetPaymentsStartBlock;
        consensus.nSuperblockStartBlock = nSuperblockStartBlock;
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
            return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
            return testNetParams;
    else if (chain == CBaseChainParams::DEVNET) {
            assert(devNetParams);
            return *devNetParams;
    } else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    if (network == CBaseChainParams::DEVNET) {
        devNetParams = (CDevNetParams*)new uint8_t[sizeof(CDevNetParams)];
        memset(devNetParams, 0, sizeof(CDevNetParams));
        new (devNetParams) CDevNetParams();
    }

    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout);
}

void UpdateRegtestBudgetParameters(int nMasternodePaymentsStartBlock, int nBudgetPaymentsStartBlock, int nSuperblockStartBlock)
{
    regTestParams.UpdateBudgetParameters(nMasternodePaymentsStartBlock, nBudgetPaymentsStartBlock, nSuperblockStartBlock);
}

void UpdateDevnetSubsidyAndDiffParams(int nMinimumDifficultyBlocks, int nHighSubsidyBlocks, int nHighSubsidyFactor)
{
    assert(devNetParams);
    devNetParams->UpdateSubsidyAndDiffParams(nMinimumDifficultyBlocks, nHighSubsidyBlocks, nHighSubsidyFactor);
}
