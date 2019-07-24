// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PacketHandler.h"

class UNetDriver;

DECLARE_LOG_CATEGORY_EXTERN(LogHandshake, Log, All);


/**
 * Forward Declarations
 */

class UNetConnection;
class UNetDriver;


/**
 * Defines
 */

#define SECRET_BYTE_SIZE 64
#define SECRET_COUNT 2


/**
 * PacketHandler component for implementing a stateless (non-memory-consuming) connection handshake
 *
 * Partially based on the Datagram Transport Layer Security protocol.
 */
class StatelessConnectHandlerComponent : public HandlerComponent
{
public:
	/**
	 * Base constructor
	 */
	StatelessConnectHandlerComponent();


	/**
	 * Constructs and sends the initial connect packet, from the client to the server
	 */
	ENGINE_API void SendInitialConnect();

private:
	/**
	 * Constructs and sends the server response to the initial connect packet, from the server to the client.
	 *
	 * @param ClientAddress		The address of the client to send the challenge to.
	 */
	void SendConnectChallenge(FString ClientAddress);

	/**
	 * Constructs and sends the final handshake challenge response packet, from the client to the server
	 *
	 * @param InSecretId		Which of the two server HandshakeSecret values this uses
	 * @param InTimestamp		The timestamp value to send
	 * @param InCookie			The cookie value to send
	 */
	void SendChallengeResponse(uint8 InSecretId, float InTimestamp, uint8 InCookie[20]);


	/**
	 * Pads the handshake packet, to match the PacketBitAlignment of the PacketHandler, so that it will parse correctly.
	 *
	 * @param HandshakePacket	The handshake packet to be aligned.
	 */
	void CapHandshakePacket(FBitWriter& HandshakePacket);

public:
	/**
	 * Whether or not the specified connection address, has just passed the connection handshake challenge.
	 *
	 * @param Address	The address (including port, for UIpNetDriver) being checked
	 */
	FORCEINLINE bool HasPassedChallenge(FString Address)
	{
		return LastChallengeSuccessAddress == Address;
	}


	/**
	 * Sets the net driver this handler is associated with
	 *
	 * @param InDriver	The net driver to set
	 */
	void SetDriver(UNetDriver* InDriver);


protected:
	virtual void Initialize() override;

	virtual void Incoming(FBitReader& Packet) override;

	virtual void Outgoing(FBitWriter& Packet) override;

	virtual void IncomingConnectionless(FString Address, FBitReader& Packet) override;

	virtual void OutgoingConnectionless(FString Address, FBitWriter& Packet) override
	{
	}

	virtual bool CanReadUnaligned() override
	{
		return true;
	}

	virtual int32 GetReservedPacketBits() override;

	virtual void Tick(float DeltaTime) override;

private:
	/**
	 * Parses an incoming handshake packet (does not parse the handshake bit though)
	 *
	 * @param Packet		The packet the handshake is being parsed from
	 * @param OutSecretId	Which of the two serverside HandshakeSecret values this is based on
	 * @param OutTimestamp	The server timestamp, from the moment the challenge was sent (or 0.f if from the client)
	 * @param OutCookie		A unique identifier, generated by the server, which the client must reply with (or 0, for initial packet)
	 * @return				Whether or not the handshake packet was parsed successfully
	 */
	bool ParseHandshakePacket(FBitReader& Packet, uint8& OutSecretId, float& OutTimestamp, uint8 (&OutCookie)[20]);

	/**
	 * Takes the client address plus server timestamp, and outputs a deterministic cookie value
	 *
	 * @param ClientAddress		The address of the client, including port
	 * @param SecretId			Which of the two HandshakeSecret values the cookie will be based on
	 * @param TimeStamp			The serverside timestamp
	 * @param OutCookie			Outputs the generated cookie value.
	 */
	void GenerateCookie(FString ClientAddress, uint8 SecretId, float Timestamp, uint8 (&OutCookie)[20]);

	/**
	 * Generates a new HandshakeSecret value
	 */
	void UpdateSecret();


private:
	/** The net driver associated with this handler - for performing connectionless sends */
	UNetDriver* Driver;


	/** Serverside variables */

	/** The serverside-only 'secret' value, used to help with generating cookies. */
	TArray<uint8> HandshakeSecret[SECRET_COUNT];

	/** Which of the two secret values above is active (values are changed frequently, to limit replay attacks) */
	uint8 ActiveSecret;

	/** The time of the last secret value update */
	float LastSecretUpdateTimestamp;

	/** The last address to successfully complete the handshake challenge */
	FString LastChallengeSuccessAddress;


	/** Clientside variables */

	/** The last time a handshake packet was sent - used for detecting failed sends. */
	double LastClientSendTimestamp;

	/** Whether or not the connection has been confirmed as active. */
	bool bConnectConfirmed;


	/** The SecretId value of the last challenge response sent */
	uint8 LastSecretId;

	/** The Timestamp value of the last challenge response sent */
	float LastTimestamp;

	/** The Cookie value of the last challenge response sent */
	uint8 LastCookie[20];
};
