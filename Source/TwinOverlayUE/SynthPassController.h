// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CommonStructs.h"
#include "SynthPassController.generated.h"

class UMediaPlayer;
class UMediaSource;
class UMediaTexture;
class UTextureRenderTarget2D;

/**
 * Drives the SynthPass compositor for the level it is placed in.
 * Actors listed in ForegroundActors are rendered over the input footage; everything else is treated as virtual world.
 */
UCLASS(Blueprintable)
class TWINOVERLAYUE_API ASynthPassController : public AActor
{
	GENERATED_BODY()

public:
	ASynthPassController();

	// Actors composited over the footage (written to the SynthPass custom stencil mask)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SynthPass|Foreground")
	TArray<TObjectPtr<AActor>> ForegroundActors;

	// When ForegroundActors is empty, actors whose class name contains this string are made foreground on start
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Foreground")
	FString ForegroundClassNameFilter = TEXT("Rack");

	// When ForegroundActors is empty, actors with this tag are made foreground on start
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Foreground")
	FName ForegroundTag = TEXT("SynthPassForeground");

	// Movie played as the background plate when MediaSource is not set. Relative paths resolve against the Content folder
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Input")
	FString MovieFilePath = TEXT("Movies/vidssave.com AVTECH HD CCTV Video On Warehouse 720P.mp4");

	// Media texture used as the background plate. Created at runtime if not set
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Input")
	TObjectPtr<UMediaTexture> InputMediaTexture;

	// Optional: player feeding InputMediaTexture, opened with MediaSource on BeginPlay
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Input")
	TObjectPtr<UMediaPlayer> MediaPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Input")
	TObjectPtr<UMediaSource> MediaSource;

	// Ignore the footage and output foreground actors on a transparent background (alpha matte)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Input")
	bool bTransparentBackground = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Output")
	FCompositorOutputSpecs OutputSpecs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Output")
	bool bOutputToViewport = true;

	// Weight of the foreground CG drawn over the footage
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Weights", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ForegroundWeight = 1.f;

	// Weight of the full virtual world (warehouse). 0 hides every non-foreground actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Weights", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VirtualWorldWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass")
	bool bAutoStart = true;

	// Animates VirtualWorldWeight: footage -> warehouse (-> footage when looping). Foreground actors stay on top throughout
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Transition")
	bool bAutoTransition = true;

	// Seconds of pure footage (plus foreground) before the warehouse fades in
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Transition", meta = (ClampMin = "0.0"))
	float VideoHoldTime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Transition", meta = (ClampMin = "0.1"))
	float TransitionDuration = 4.f;

	// Seconds the full warehouse is held before fading back (when looping)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Transition", meta = (ClampMin = "0.0"))
	float WarehouseHoldTime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SynthPass|Transition")
	bool bLoopTransition = true;

	UFUNCTION(BlueprintCallable, Category = "SynthPass")
	void StartSynthPass();

	UFUNCTION(BlueprintCallable, Category = "SynthPass")
	void StopSynthPass();

	// Adds or removes a single actor from the foreground set
	UFUNCTION(BlueprintCallable, Category = "SynthPass|Foreground")
	void SetActorForeground(AActor* Actor, bool bForeground);

	UFUNCTION(BlueprintCallable, Category = "SynthPass|Foreground")
	void SetForegroundActors(const TArray<AActor*>& Actors);

	UFUNCTION(BlueprintPure, Category = "SynthPass|Foreground")
	bool IsActorForeground(const AActor* Actor) const;

	// Re-applies weights and the foreground set to the running compositor
	UFUNCTION(BlueprintCallable, Category = "SynthPass")
	void ApplySettings();

	// Sets the virtual world weight on the running compositor without re-applying the foreground set
	UFUNCTION(BlueprintCallable, Category = "SynthPass|Weights")
	void SetVirtualWorldWeight(float Weight);

	UFUNCTION(BlueprintCallable, Category = "SynthPass|Transition")
	void RestartTransition();

	UFUNCTION(BlueprintPure, Category = "SynthPass|Output")
	UTextureRenderTarget2D* GetOutputRenderTarget() const;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Creates the media source/player/texture from MovieFilePath where they are not set
	void SetupMedia();

	// Fills ForegroundActors from ForegroundClassNameFilter and ForegroundTag
	void CollectForegroundActors();

	bool bRunning = false;
	float TransitionTime = 0.f;
};
