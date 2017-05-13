/**
 * @file       Coin.h
 *
 * @brief      PublicCoin and PrivateCoin classes for the Zerocoin library.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/

#ifndef COIN_H_
#define COIN_H_
#include "bignum.h"
#include "amount.h"
#include "Params.h"
#include "../util.h"

namespace libzerocoin {

//PRESSTAB: should we add an invalid representation for CoinDenomination?
enum  CoinDenomination {
    ZQ_LOVELACE = 1,
    ZQ_GOLDWASSER = 10,
    ZQ_RACKOFF = 25,
    ZQ_PEDERSEN = 50,
    ZQ_WILLIAMSON = 100, // Malcolm J. Williamson,
                    // the scientist who actually invented
                    // Public key cryptography
    ZQ_ERROR = 0
};

inline bool AmountToZerocoinDenomination(uint256 amount, CoinDenomination& denomination)
{
	if(amount == CoinDenomination::ZQ_LOVELACE * COIN)
		denomination = CoinDenomination::ZQ_LOVELACE;
	else if(amount == CoinDenomination::ZQ_GOLDWASSER * COIN)
		denomination = CoinDenomination ::ZQ_GOLDWASSER;
	else if(amount == CoinDenomination::ZQ_RACKOFF * COIN)
		denomination = CoinDenomination::ZQ_RACKOFF;
	else if(amount == CoinDenomination::ZQ_PEDERSEN * COIN)
		denomination = CoinDenomination::ZQ_PEDERSEN;
	else if(amount == CoinDenomination::ZQ_WILLIAMSON * COIN)
		denomination = CoinDenomination::ZQ_WILLIAMSON;
	else
	{
		//not a valid denomination mark to minimal and return false
		//should mark invalid if we add that denom to the enum
		denomination = CoinDenomination::ZQ_ERROR;
		return false;
	}

    return true;
}

    
inline int64_t roundint64(double d)
{
    return (int64_t)(d > 0 ? d + 0.5 : d - 0.5);
}

    
inline CoinDenomination get_denomination(string denomAmount) {
    CoinDenomination denomination;
    // Amount
    if (denomAmount == "1") {
        denomination = ZQ_LOVELACE;
    } else if (denomAmount == "10") {
        denomination = ZQ_GOLDWASSER;
    } else if (denomAmount == "25") {
        denomination = ZQ_RACKOFF;
    } else if (denomAmount == "50") {
        denomination = ZQ_PEDERSEN;
    } else if (denomAmount == "100") {
        denomination = ZQ_WILLIAMSON;
    } else {
        // SHOULD WE THROW EXCEPTION or Something?
        denomination = ZQ_ERROR; // ERROR HACK(SPOCK)???
    }
    return denomination;
}


inline int64_t get_amount(string denomAmount) {
    int64_t nAmount = 0;
    // Amount
    if (denomAmount == "1") {
        nAmount = roundint64(1 * COIN);
    } else if (denomAmount == "10") {
        nAmount = roundint64(10 * COIN);
    } else if (denomAmount == "25") {
        nAmount = roundint64(25 * COIN);
    } else if (denomAmount == "50") {
        nAmount = roundint64(50 * COIN);
    } else if (denomAmount == "100") {
        nAmount = roundint64(100 * COIN);
    } else {
        // SHOULD WE THROW EXCEPTION or Something?
        nAmount = 0;
    }
    return nAmount;
}


/** A Public coin is the part of a coin that
 * is published to the network and what is handled
 * by other clients. It contains only the value
 * of commitment to a serial number and the
 * denomination of the coin.
 */
class PublicCoin {
public:
	template<typename Stream>
	PublicCoin(const Params* p, Stream& strm): params(p) {
		strm >> *this;
	}

	PublicCoin( const Params* p);

	/**Generates a public coin
	 *
	 * @param p cryptographic paramters
	 * @param coin the value of the commitment.
	 * @param denomination The denomination of the coin. Defaults to ZQ_LOVELACE
	 */
	PublicCoin( const Params* p, const CBigNum& coin, const CoinDenomination d = ZQ_LOVELACE);
	const CBigNum& getValue() const;
	CoinDenomination getDenomination() const;
	bool operator==(const PublicCoin& rhs) const;
	bool operator!=(const PublicCoin& rhs) const;
	/** Checks that a coin prime
	 *  and in the appropriate range
	 *  given the parameters
	 * @return true if valid
	 */
    bool validate() const;
	
	ADD_SERIALIZE_METHODS;
  template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
	    READWRITE(value);
	    READWRITE(denomination);
	}	
private:
	const Params* params;
	CBigNum value;
	// Denomination is stored as an INT because storing
	// and enum raises amigiuities in the serialize code //FIXME if possible
	int denomination;
};

/**
 * A private coin. As the name implies, the content
 * of this should stay private except PublicCoin.
 *
 * Contains a coin's serial number, a commitment to it,
 * and opening randomness for the commitment.
 *
 * @warning Failure to keep this secret(or safe),
 * @warning will result in the theft of your coins
 * @warning and a TOTAL loss of anonymity.
 */
class PrivateCoin {
public:
	template<typename Stream>
	PrivateCoin(const Params* p, Stream& strm): params(p),publicCoin(p) {
		strm >> *this;
	}
	PrivateCoin(const Params* p,const CoinDenomination denomination = ZQ_LOVELACE);
	const PublicCoin& getPublicCoin() const;
	const CBigNum& getSerialNumber() const;
	const CBigNum& getRandomness() const;
    
  void setPublicCoin(PublicCoin p){
      publicCoin = p;
  }

  void setRandomness(Bignum n){
      randomness = n;
  }

  void setSerialNumber(Bignum n){
      serialNumber = n;
  }

	ADD_SERIALIZE_METHODS;
  template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
	    READWRITE(publicCoin);
	    READWRITE(randomness);
	    READWRITE(serialNumber);
	}
private:
	const Params* params;
	PublicCoin publicCoin;
	CBigNum randomness;
	CBigNum serialNumber;

	/**
	 * @brief Mint a new coin.
	 * @param denomination the denomination of the coin to mint
	 * @throws ZerocoinException if the process takes too long
	 *
	 * Generates a new Zerocoin by (a) selecting a random serial
	 * number, (b) committing to this serial number and repeating until
	 * the resulting commitment is prime. Stores the
	 * resulting commitment (coin) and randomness (trapdoor).
	 **/
	void mintCoin(const CoinDenomination denomination);
	
	/**
	 * @brief Mint a new coin using a faster process.
	 * @param denomination the denomination of the coin to mint
	 * @throws ZerocoinException if the process takes too long
	 *
	 * Generates a new Zerocoin by (a) selecting a random serial
	 * number, (b) committing to this serial number and repeating until
	 * the resulting commitment is prime. Stores the
	 * resulting commitment (coin) and randomness (trapdoor).
	 * This routine is substantially faster than the
	 * mintCoin() routine, but could be more vulnerable
	 * to timing attacks. Don't use it if you think someone
	 * could be timing your coin minting.
	 **/
	void mintCoinFast(const CoinDenomination denomination);

};

} /* namespace libzerocoin */
#endif /* COIN_H_ */
