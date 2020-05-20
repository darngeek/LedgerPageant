// decompressionn of pubkey
#include <cryptopp/cryptlib.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>
#include <cryptopp/hex.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/base64.h>

namespace encodeUtils {
	static std::string makeSha256(std::string msg) {
		std::string digest;
		CryptoPP::SHA256 sha256;
		CryptoPP::HashFilter f3(sha256, new CryptoPP::StringSink(digest));

		CryptoPP::ChannelSwitch cs;
		cs.AddDefaultRoute(f3);

		CryptoPP::StringSource ss(msg, true /*pumpAll*/, new CryptoPP::Redirector(cs));

		return digest;
	}

	static std::string encodeBase64(const std::string& raw_string) {
		CryptoPP::Base64Encoder encoder(nullptr, false, 1);
		encoder.Put((const CryptoPP::byte*)raw_string.c_str(), raw_string.size());
		encoder.MessageEnd();

		std::string encoded;
		CryptoPP::word64 size = encoder.MaxRetrievable();
		if (size) {
			encoded.resize(size);
			encoder.Get((CryptoPP::byte*) & encoded[0], encoded.size());
			return encoded;
		}

		return {};
	}

	static ByteArray decompressPubKey(const ByteArray& inCompressedKey) {
		/*
			eventho romain is using NIST256p1
			https://tools.ietf.org/search/rfc4492 indicates we can use:
			secp256r1   |  prime256v1   |   NIST P-256
		*/

		CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey _pubKey;
		_pubKey.AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256r1());

		CryptoPP::ArraySource compressedSource(inCompressedKey.Get().data(), inCompressedKey.Size(), true);

		CryptoPP::ECP::Point point;
		_pubKey.GetGroupParameters().GetCurve().DecodePoint(point, compressedSource, compressedSource.MaxRetrievable());

		ByteArray pointMash;

		uint32_t numXbytes = point.x.ByteCount();
		for (uint32_t i = 1; i <= numXbytes; ++i) {
			pointMash.PushBack(point.x.GetByte(numXbytes - i));
		}

		uint32_t numYbytes = point.y.ByteCount();
		for (uint32_t i = 1; i <= numYbytes; ++i) {
			pointMash.PushBack(point.y.GetByte(numYbytes - i));
		}

		return pointMash;
	}
}
