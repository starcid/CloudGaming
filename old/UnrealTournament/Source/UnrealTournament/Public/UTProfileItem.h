// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "EngineBuildSettings.h"
#include "UTCharacterContent.h"
#include "UTCosmetic.h"

#if WITH_PROFILE
#include "UtMcpDefinition.h"
#else
#include "GithubStubs.h"
#endif

#include "UTProfileItem.generated.h"

USTRUCT()
struct FProfileItemEntry
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	const class UUTProfileItem* Item;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Count;

	FProfileItemEntry()
		: Item(NULL), Count(0)
	{}
	FProfileItemEntry(const UUTProfileItem* InItem, uint32 InCount)
	: Item(InItem), Count(int32(FMath::Min<uint32>(InCount, MAX_int32)))
	{}
};

USTRUCT()
struct FCosmeticEntry
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Meta = (MetaClass = "UTCosmetic"))
	FStringClassReference Item;
	/** optional variant ID for cosmetic items that support it */
	UPROPERTY(EditAnywhere)
	int32 VariantId;

	FCosmeticEntry()
		: VariantId(0)
	{}
	explicit FCosmeticEntry(const FStringClassReference& InItem, int32 InVariant = 0)
		: Item(InItem), VariantId(InVariant)
	{}
};

/** collectable/consumable item that a player owns in their profile, such as a hat */
UCLASS()
class UNREALTOURNAMENT_API UUTProfileItem : public UUtMcpDefinition
{
	GENERATED_BODY()
public:

	UUTProfileItem(const FObjectInitializer& OI)
	: Super(OI)
	{
		bTradable = true;
#if WITH_PROFILE
		ItemType = EUtItemType::Item;
#endif
	}

	/** whether this item can be traded to others */
	UPROPERTY(EditAnywhere)
	bool bTradable;
	/** if true, player can only have one of this item */
	UPROPERTY(EditAnywhere)
	bool bUnique;
	/** human readable name */
	UPROPERTY(EditAnywhere)
	FText DisplayName;
	/** art for UI */
	UPROPERTY(EditAnywhere)
	FCanvasIcon Image;
	/** player can construct this item by combining these other items */
	UPROPERTY(EditAnywhere)
	TArray<FProfileItemEntry> Recipe;
	
	UPROPERTY()
	TArray<FStringClassReference> GrantedCosmetics_DEPRECATED;
	/** when this item is in the player's inventory, they have access to these cosmetic items */
	UPROPERTY(EditAnywhere)
	TArray<FCosmeticEntry> GrantedCosmeticItems;
	/** when this item is in the player's inventory, they have access to these player characters */
	UPROPERTY(EditAnywhere, Meta = (MetaClass = "UTCharacterContent"))
	TArray<FStringClassReference> GrantedCharacters;
	/** when this item is in the player's inventory, they have access to these BOT characters */
	UPROPERTY(EditAnywhere, Meta = (AllowedClasses = "UTBotCharacter"))
	TArray<FStringAssetReference> GrantedBots;

#if WITH_EDITOR
	virtual void PreSave(const class ITargetPlatform* TargetPlatform) override
	{
		Super::PreSave(TargetPlatform);

		// if there's only one granted item, match names
		if (GIsEditor && !IsTemplate() && GrantedCosmeticItems.Num() + GrantedCharacters.Num() + GrantedBots.Num() == 1)
		{
			if (GrantedCosmeticItems.Num() == 1)
			{
				TSubclassOf<AUTCosmetic> CosmeticClass = GrantedCosmeticItems[0].Item.TryLoadClass<AUTCosmetic>();
				if (CosmeticClass != NULL && !CosmeticClass.GetDefaultObject()->CosmeticName.IsEmpty())
				{
					DisplayName = FText::FromString(CosmeticClass.GetDefaultObject()->CosmeticName);
				}
			}
			else if (GrantedCharacters.Num() == 1)
			{
				TSubclassOf<AUTCharacterContent> CharClass = GrantedCharacters[0].TryLoadClass<AUTCharacterContent>();
				if (CharClass != NULL && !CharClass.GetDefaultObject()->DisplayName.IsEmpty())
				{
					DisplayName = CharClass.GetDefaultObject()->DisplayName;
				}
			}
			else if (GrantedBots.Num() == 1)
			{
				UObject* BotPkg = NULL;
				FString BotPathname = GrantedBots[0].ToString();
				ResolveName(BotPkg, BotPathname, true, false);
				if (BotPkg != NULL)
				{
					DisplayName = FText::FromString(BotPkg->GetName());
				}
			}
		}
	}
#endif

	virtual void PostLoad() override
	{
		Super::PostLoad();

		if (GrantedCosmetics_DEPRECATED.Num() > 0)
		{
			for (const FStringClassReference& Item : GrantedCosmetics_DEPRECATED)
			{
				new(GrantedCosmeticItems) FCosmeticEntry(Item);
			}
			GrantedCosmetics_DEPRECATED.Empty();
		}
	}

	/** returns whether this item grants access to the object with the specified path */
	inline bool Grants(const FString& Path, int32 VariantId = 0) const
	{
		FString BPClassPath = Path + TEXT("_C");
		return GrantedCosmeticItems.ContainsByPredicate([&](const FCosmeticEntry& TestItem) { return TestItem.VariantId == VariantId && (TestItem.Item.ToString() == Path || TestItem.Item.ToString() == BPClassPath); }) ||
			GrantedCharacters.ContainsByPredicate([&](const FStringClassReference& TestItem) { return TestItem.ToString() == Path || TestItem.ToString() == BPClassPath; }) ||
			GrantedBots.ContainsByPredicate([&](const FStringAssetReference& TestItem) { return TestItem.ToString().Contains(Path); });
	}

	static FString TemplateType()
	{
		return TEXT("Item");
	}

	/** returns backend ID name */
	inline FString GetTemplateID() const
	{
		return FString::Printf(TEXT("%s.%s"), *TemplateType(), *GetName());
	}

	void ExportBackendJson(TSharedRef< TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR> > > Writer) const
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("templateId"), GetTemplateID());
		Writer->WriteObjectStart(TEXT("attributes"));
		Writer->WriteValue(TEXT("static_max_stack_size"), bUnique ? 1 : -1);
		Writer->WriteValue(TEXT("static_max_num_stacks"), bUnique ? 1 : -1);
		Writer->WriteValue(TEXT("tradable"), bTradable);
		if (Recipe.Num() > 0)
		{
			Writer->WriteArrayStart(TEXT("recipe"));
			for (const FProfileItemEntry& RecipeItem : Recipe)
			{
				if (RecipeItem.Item != NULL)
				{
					Writer->WriteObjectStart();
					Writer->WriteValue(TEXT("item"), RecipeItem.Item->GetTemplateID());
					Writer->WriteValue(TEXT("count"), RecipeItem.Count);
					Writer->WriteObjectEnd();
				}
			}
			Writer->WriteArrayEnd();
		}
		Writer->WriteObjectEnd();
		Writer->WriteObjectEnd();
	}
};