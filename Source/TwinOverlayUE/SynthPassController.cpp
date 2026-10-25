// Copyright Epic Games, Inc. All Rights Reserved.

#include "SynthPassController.h"
#include "CompGraphicsSubsystem.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "Misc/Paths.h"

static UCompGraphicsSubsystem* GetSynthPass()
{
	return GEngine ? GEngine->GetEngineSubsystem<UCompGraphicsSubsystem>() : nullptr;
}

ASynthPassController::ASynthPassController()
{
	PrimaryActorTick.bCanEverTick = true;
	OutputSpecs.PixelFormat = ERenderTargetPixelFormat::RTF_RGBA16F;
}

void ASynthPassController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bRunning || !bAutoTransition)
	{
		return;
	}

	TransitionTime += DeltaSeconds;

	// Timeline: footage hold -> fade in -> warehouse hold -> fade out (fade out only when looping)
	const float FadeInEnd = VideoHoldTime + TransitionDuration;
	const float HoldEnd = FadeInEnd + WarehouseHoldTime;
	const float CycleLength = HoldEnd + TransitionDuration;
	const float T = bLoopTransition ? FMath::Fmod(TransitionTime, CycleLength) : FMath::Min(TransitionTime, HoldEnd);

	float Alpha;
	if (T < VideoHoldTime)
	{
		Alpha = 0.f;
	}
	else if (T < FadeInEnd)
	{
		Alpha = (T - VideoHoldTime) / TransitionDuration;
	}
	else if (T < HoldEnd)
	{
		Alpha = 1.f;
	}
	else
	{
		Alpha = 1.f - (T - HoldEnd) / TransitionDuration;
	}

	SetVirtualWorldWeight(FMath::SmoothStep(0.f, 1.f, Alpha));
}

void ASynthPassController::SetupMedia()
{
	if (!MediaSource && !MovieFilePath.IsEmpty())
	{
		const FString FullPath = FPaths::IsRelative(MovieFilePath) ? FPaths::Combine(FPaths::ProjectContentDir(), MovieFilePath) : MovieFilePath;
		if (FPaths::FileExists(FullPath))
		{
			UFileMediaSource* FileSource = NewObject<UFileMediaSource>(this);
			FileSource->SetFilePath(FullPath);
			MediaSource = FileSource;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SynthPassController: movie not found at %s"), *FullPath);
		}
	}

	if (!MediaSource)
	{
		return;
	}

	if (!MediaPlayer)
	{
		MediaPlayer = NewObject<UMediaPlayer>(this);
		MediaPlayer->PlayOnOpen = true;
	}
	MediaPlayer->SetLooping(true);

	if (!InputMediaTexture)
	{
		InputMediaTexture = NewObject<UMediaTexture>(this);
		InputMediaTexture->AutoClear = true;
	}
	InputMediaTexture->SetMediaPlayer(MediaPlayer);
	InputMediaTexture->UpdateResource();

	MediaPlayer->OpenSource(MediaSource);
}

void ASynthPassController::CollectForegroundActors()
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == this)
		{
			continue;
		}

		const bool bTagged = !ForegroundTag.IsNone() && Actor->ActorHasTag(ForegroundTag);
		const bool bNameMatch = !ForegroundClassNameFilter.IsEmpty() && Actor->GetClass()->GetName().Contains(ForegroundClassNameFilter);
		if (bTagged || bNameMatch)
		{
			ForegroundActors.AddUnique(Actor);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("SynthPassController: %d foreground actors collected"), ForegroundActors.Num());
}

void ASynthPassController::SetVirtualWorldWeight(float Weight)
{
	Weight = FMath::Clamp(Weight, 0.f, 1.f);
	if (Weight == VirtualWorldWeight)
	{
		return;
	}
	VirtualWorldWeight = Weight;

	UCompGraphicsSubsystem* SynthPass = GetSynthPass();
	if (SynthPass && bRunning)
	{
		// At exactly 0 the subsystem hides every non-foreground actor; above 0 it unhides them
		SynthPass->SetVirtualWorldOverlayValue(VirtualWorldWeight);
	}
}

void ASynthPassController::RestartTransition()
{
	TransitionTime = 0.f;
}

void ASynthPassController::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStart)
	{
		StartSynthPass();
	}
}

void ASynthPassController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopSynthPass();
	Super::EndPlay(EndPlayReason);
}

void ASynthPassController::StartSynthPass()
{
	UCompGraphicsSubsystem* SynthPass = GetSynthPass();
	if (!SynthPass || bRunning)
	{
		return;
	}

	SetupMedia();

	if (ForegroundActors.IsEmpty())
	{
		CollectForegroundActors();
	}

	if (bAutoTransition)
	{
		TransitionTime = 0.f;
		VirtualWorldWeight = 0.f;
	}

	SynthPass->InitilizeCompositor();
	SynthPass->ConfigureCompositorOutput(OutputSpecs);
	SynthPass->SetInputMediaTexture(InputMediaTexture, bTransparentBackground);
	SynthPass->ToggleCompOutputOnViewport(bOutputToViewport);
	bRunning = true;
	ApplySettings();
	SynthPass->StartCompositor();

	if (!InputMediaTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("SynthPassController: no InputMediaTexture set, compositor will pass the scene through"));
	}
}

void ASynthPassController::StopSynthPass()
{
	UCompGraphicsSubsystem* SynthPass = GetSynthPass();
	if (!SynthPass || !bRunning)
	{
		return;
	}

	SynthPass->ResetShowOnlyActors();
	SynthPass->SetVirtualWorldOverlayValue(1.f); // Unhides actors hidden by the foreground isolation
	SynthPass->EndCompositor();
	SynthPass->DeinitializeCompositor();

	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}
	bRunning = false;
}

void ASynthPassController::SetActorForeground(AActor* Actor, bool bForeground)
{
	if (!Actor)
	{
		return;
	}

	if (bForeground)
	{
		ForegroundActors.AddUnique(Actor);
	}
	else
	{
		ForegroundActors.Remove(Actor);
	}
	ApplySettings();
}

void ASynthPassController::SetForegroundActors(const TArray<AActor*>& Actors)
{
	ForegroundActors.Reset();
	for (AActor* Actor : Actors)
	{
		if (Actor)
		{
			ForegroundActors.AddUnique(Actor);
		}
	}
	ApplySettings();
}

bool ASynthPassController::IsActorForeground(const AActor* Actor) const
{
	return Actor && ForegroundActors.Contains(Actor);
}

void ASynthPassController::ApplySettings()
{
	UCompGraphicsSubsystem* SynthPass = GetSynthPass();
	if (!SynthPass || !bRunning)
	{
		return;
	}

	TArray<AActor*> ValidActors;
	for (AActor* Actor : ForegroundActors)
	{
		if (IsValid(Actor))
		{
			ValidActors.Add(Actor);
		}
	}

	// SetShowOnlyActors hides every other actor; the virtual world weight below decides whether they come back
	SynthPass->SetShowOnlyActors(ValidActors);
	SynthPass->SetCGOverlayonBackgroundFootageValue(ForegroundWeight);
	SynthPass->SetVirtualWorldOverlayValue(VirtualWorldWeight);
	SynthPass->ToggleCompOutputOnViewport(bOutputToViewport);
}

UTextureRenderTarget2D* ASynthPassController::GetOutputRenderTarget() const
{
	UCompGraphicsSubsystem* SynthPass = GetSynthPass();
	return SynthPass ? SynthPass->GetCompositionOutputRenderTarget() : nullptr;
}
