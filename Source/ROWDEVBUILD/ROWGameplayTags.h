// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "NativeGameplayTags.h"

// Gameplay tags defined in code, so they exist without editing Project Settings.
namespace ROWTags
{
	// Granted by the Frost slow effect. Enemies with this tag move at reduced speed.
	ROWDEVBUILD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Slowed);
}