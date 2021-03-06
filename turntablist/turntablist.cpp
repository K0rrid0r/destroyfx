/*------------------------------------------------------------------------
Copyright (c) 2004 bioroid media development & Copyright (C) 2004-2020 Sophia Poirier
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. 
* Neither the name of "bioroid media development" nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Additional changes since June 15th 2004 have been made by Sophia Poirier.  

This file is part of Turntablist.

Turntablist is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

You should have received a copy of the GNU General Public License 
along with Turntablist.  If not, see <http://www.gnu.org/licenses/>.

To contact the developer, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "turntablist.h"

#include <algorithm>
#include <cmath>

#include "dfxmath.h"
#include "dfxmisc.h"



/////////////////////////////
// DEFINES

// allow pitch bend scratching
#define USE_MIDI_PITCH_BEND
// use hard-coded MIDI CCs for parameters
//#define USE_MIDI_CC

const long k_nScratchInterval = 16; //24
const long k_nPowerInterval = 120;
const long k_nRootKey_default = 60;   // middle C

const double k_fScratchAmountMiddlePoint = 0.0;
const double k_fScratchAmountLaxRange = 0.000000001;
const double k_fScratchAmountMiddlePoint_UpperLimit = k_fScratchAmountMiddlePoint + k_fScratchAmountLaxRange;
const double k_fScratchAmountMiddlePoint_LowerLimit = k_fScratchAmountMiddlePoint - k_fScratchAmountLaxRange;



#ifdef USE_LIBSNDFILE
	#include "sndfile.h"
#else
	#include <AudioToolbox/ExtendedAudioFile.h>
#endif



//-----------------------------------------------------------------------------------------
// Turntablist
//-----------------------------------------------------------------------------------------
// this macro does boring entry point stuff for us
DFX_EFFECT_ENTRY(Turntablist)

//-----------------------------------------------------------------------------------------
Turntablist::Turntablist(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, 0)
{
	initparameter_f(kParam_ScratchAmount, {"scratch amount"}, k_fScratchAmountMiddlePoint, k_fScratchAmountMiddlePoint, -1.0, 1.0, DfxParam::Unit::Generic);	// float2string(m_fPlaySampleRate, text);
//	initparameter_f(kScratchSpeed, {"scratch speed"}, 0.33333333, 0.33333333, 0.0, 1.0, DfxParam::Unit::Scalar);
	initparameter_f(kParam_ScratchSpeed_scrub, {"scratch speed (scrub mode)"}, 2.0, 2.0, 0.5, 5.0, DfxParam::Unit::Seconds);
	initparameter_f(kParam_ScratchSpeed_spin, {"scratch speed (spin mode)"}, 3.0, 3.0, 1.0, 8.0, DfxParam::Unit::Scalar);

	initparameter_b(kParam_Power, {"power"}, true, true);
	initparameter_f(kParam_SpinUpSpeed, {"spin up speed"}, 0.063, 0.05, 0.0001, 1.0, DfxParam::Unit::Scalar, DfxParam::Curve::Log);
	initparameter_f(kParam_SpinDownSpeed, {"spin down speed"}, 0.09, 0.05, 0.0001, 1.0, DfxParam::Unit::Scalar, DfxParam::Curve::Log);
	initparameter_b(kParam_NotePowerTrack, {"note-power track"}, false, false);

	initparameter_f(kParam_PitchShift, {"pitch shift"}, 0.0, 0.0, -100.0, 100.0, DfxParam::Unit::Percent);
	initparameter_f(kParam_PitchRange, {"pitch range"}, 12.0, 12.0, 1.0, 36.0, DfxParam::Unit::Semitones);
	initparameter_b(kParam_KeyTracking, {"key track"}, false, false);
	initparameter_i(kParam_RootKey, {"root key"}, k_nRootKey_default, k_nRootKey_default, 0, 0x7F, DfxParam::Unit::Notes);

	initparameter_b(kParam_Loop, {"loop"}, true, false);

	initparameter_list(kParam_ScratchMode, {"scratch mode"}, kScratchMode_Scrub, kScratchMode_Scrub, kNumScratchModes);
	setparametervaluestring(kParam_ScratchMode, kScratchMode_Scrub, "scrub");
	setparametervaluestring(kParam_ScratchMode, kScratchMode_Spin, "spin");

	initparameter_list(kParam_Direction, {"playback direction"}, kScratchDirection_Forward, kScratchDirection_Forward, kNumScratchDirections);
	setparametervaluestring(kParam_Direction, kScratchDirection_Forward, "forward");
	setparametervaluestring(kParam_Direction, kScratchDirection_Backward, "reverse");

	initparameter_list(kParam_NoteMode, {"note mode"}, kNoteMode_Reset, kNoteMode_Reset, kNumNoteModes);
	setparametervaluestring(kParam_NoteMode, kNoteMode_Reset, "reset");
	setparametervaluestring(kParam_NoteMode, kNoteMode_Resume, "resume");

	initparameter_b(kParam_PlayTrigger, {"playback trigger"}, false, false);

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	initparameter_b(kParam_Mute, {"mute"}, false, false);
	initparameter_f(kParam_Volume, {"volume"}, 1.0, 0.5, 0.0, 1.0, DfxParam::Unit::LinearGain, DfxParam::Curve::Cubed);
#endif


#ifdef USE_LIBSNDFILE
	addchannelconfig(0, 2);	// stereo-out
	addchannelconfig(0, 1);	// mono-out
#else
	addchannelconfig(0, -1);	// 0-in / N-out
#endif

#if LOGIC_AU_PROPERTIES_AVAILABLE
	// XXX This plugin doesn't support nodes because of the problem of passing a file reference 
	// as custom property data between different machines on a network.  
	// There is no reliable way to do that, so far as I know.
	setSupportedLogicNodeOperationMode(0);
#endif

	m_nNumChannels = 0;
	m_nNumSamples = 0;
	m_nSampleRate = 0;
	m_fSampleRate = 0.0;


	m_bPlayedReverse = false;
	m_bPlay = false;

	m_fPosition = 0.0;
	m_fPosOffset = 0.0;
	m_fNumSamples = 0.0;

	m_fLastScratchAmount = getparameter_f(kParam_ScratchAmount);
	m_fPitchBend = k_fScratchAmountMiddlePoint;
	m_bPitchBendSet = false;
	m_bScratching = false;
	m_bWasScratching = false;
	m_bPlayForward = true;
	m_nScratchSubMode = 1;	// speed based from scrub scratch mode

	m_nScratchDelay = 0;

	m_nRootKey = static_cast<int>(getparameter_i(kParam_RootKey));

	m_nCurrentNote = m_nRootKey;
	m_nCurrentVelocity = 0x7F;

	m_bAudioFileHasBeenLoaded = false;
	m_bScratchStop = false;

	memset(&m_fsAudioFile, 0, sizeof(m_fsAudioFile));

	m_fScratchVolumeModifier = 0.0f;

	m_bScratchAmountSet = false;
	m_fDesiredScratchRate = 0.0;

	m_nScratchInterval = 0;

	m_fDesiredPosition = 0.0;
	m_fPrevDesiredPosition = 0.0;
}

//-----------------------------------------------------------------------------------------
void Turntablist::dfx_PostConstructor()
{
	// XXX yah?  for now...
	getsettings().setSteal(true);
	getsettings().setUseChannel(false);
}


//-----------------------------------------------------------------------------------------
long Turntablist::initialize()
{
	if (sampleRateChanged())
	{
		m_nPowerIntervalEnd = static_cast<int>(getsamplerate()) / k_nPowerInterval;  // set process interval to 10 times a second - should maybe update it to 60?
		m_nScratchIntervalEnd = static_cast<int>(getsamplerate()) / k_nScratchInterval;  // set scratch interval end to 1/16 second
		m_nScratchIntervalEndBase = m_nScratchIntervalEnd;
	}

	stopNote();

	return dfx::kStatus_NoError;
}


//-----------------------------------------------------------------------------------------
void Turntablist::processparameters()
{
	m_bPower = getparameter_b(kParam_Power);
	m_bNotePowerTrack = getparameter_b(kParam_NotePowerTrack);
	m_bLoop = getparameter_b(kParam_Loop);
	m_bKeyTracking = getparameter_b(kParam_KeyTracking);

	m_fPitchShift = getparameter_scalar(kParam_PitchShift);
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	m_fVolume = getparameter_f(kParam_Volume);
#endif
	m_fPitchRange = getparameter_f(kParam_PitchRange);

	m_fScratchAmount = getparameter_f(kParam_ScratchAmount);
//	m_fScratchSpeed = getparameter_f(kScratchSpeed);
	m_fScratchSpeed_scrub = getparameter_f(kParam_ScratchSpeed_scrub);
	m_fScratchSpeed_spin = getparameter_f(kParam_ScratchSpeed_spin);
	m_fSpinUpSpeed = getparameter_f(kParam_SpinUpSpeed);
	m_fSpinDownSpeed = getparameter_f(kParam_SpinDownSpeed);

	m_nRootKey = static_cast<int>(getparameter_i(kParam_RootKey));

	m_nDirection = getparameter_i(kParam_Direction);
	m_nScratchMode = getparameter_i(kParam_ScratchMode);
	m_nNoteMode = getparameter_i(kParam_NoteMode);

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	m_bMute = getparameter_b(kParam_Mute);
#endif
	bool playbackTrigger = getparameter_b(kParam_PlayTrigger);


	if (getparameterchanged(kParam_PitchShift))
		processPitch();

	if (getparameterchanged(kParam_ScratchAmount) || m_bPitchBendSet)	// XXX checking m_bPitchBendSet until I fix getparameterchanged()
	{
		m_bScratchAmountSet = true;
		if ( (m_fScratchAmount > k_fScratchAmountMiddlePoint_LowerLimit) && (m_fScratchAmount < k_fScratchAmountMiddlePoint_UpperLimit) )
			m_bScratching = false;
		else
		{
			if (!m_bScratching) // first time in so init stuff here
			{
				m_fPrevDesiredPosition = m_fPosition;
				m_fDesiredScratchRate2 = m_fPlaySampleRate;
				m_fScratchCenter = m_fPosition;
				m_nScratchCenter = static_cast<int>(m_fPosition);
			}
			m_bScratching = true;
		}
		processScratch();
	}

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	m_fNoteVolume = m_fVolume * static_cast<float>(m_nCurrentVelocity) * DfxMidi::kValueScalar;
#else
	m_fNoteVolume = static_cast<float>(m_nCurrentVelocity) * DfxMidi::kValueScalar;
#endif

	if (getparameterchanged(kParam_PitchRange))
		processPitch();

	if (getparameterchanged(kParam_Direction))
		processDirection();

	calculateSpinSpeeds();

	if ( getparametertouched(kParam_PlayTrigger) )
		playNote(playbackTrigger);
}



#pragma mark -
#pragma mark plugin state

static const CFStringRef kTurntablistPreset_AudioFileReferenceKey = CFSTR("audiofile");
static const CFStringRef kTurntablistPreset_AudioFileAliasKey = CFSTR("DestroyFX-audiofile-alias");

//-----------------------------------------------------------------------------------------
OSStatus Turntablist::SaveState(CFPropertyListRef* outData)
{
	auto const status = AUBase::SaveState(outData);
	if (status != noErr)
	{
		return status;
	}

	auto const dict = (CFMutableDictionaryRef)(*outData);


// save the path of the loaded audio file, if one is currently loaded
	if (FSIsFSRefValid(&m_fsAudioFile))
	{
		dfx::UniqueCFType const audioFileUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, &m_fsAudioFile);
		if (audioFileUrl)
		{
			dfx::UniqueCFType const audioFileUrlString = CFURLCopyFileSystemPath(audioFileUrl.get(), kCFURLPOSIXPathStyle);
			if (audioFileUrlString)
			{
				auto const audioFileUrlString_ptr = audioFileUrlString.get();
				dfx::UniqueCFType const fileReferencesDictionary = CFDictionaryCreate(kCFAllocatorDefault, (void const**)(&kTurntablistPreset_AudioFileReferenceKey), (void const**)(&audioFileUrlString_ptr), 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
				if (fileReferencesDictionary)
				{
					CFDictionarySetValue(dict, CFSTR(kAUPresetExternalFileRefs), fileReferencesDictionary.get());
				}
			}
		}

		// also save an alias of the loaded audio file, as a fall-back
		AliasHandle aliasHandle = nullptr;
		Size aliasSize = 0;
		auto const aliasStatus = createAudioFileAlias(&aliasHandle, &aliasSize);
		if (aliasStatus == noErr)
		{
			aliasSize = GetAliasSize(aliasHandle);
			if (aliasSize > 0)
			{
				dfx::UniqueCFType const aliasCFData = CFDataCreate(kCFAllocatorDefault, (UInt8*)(*aliasHandle), (CFIndex)aliasSize);
				if (aliasCFData)
				{
					CFDictionarySetValue(dict, kTurntablistPreset_AudioFileAliasKey, aliasCFData.get());
				}
			}
			DisposeHandle((Handle)aliasHandle);
		}
	}


// save the MIDI CC -> parameter assignments
	getsettings().saveMidiAssignmentsToDictionary(dict);

	return noErr;
}

//-----------------------------------------------------------------------------------------
OSStatus Turntablist::RestoreState(CFPropertyListRef inData)
{
	auto const status = AUBase::RestoreState(inData);
	if (status != noErr)
	{
		return status;
	}

	auto const dict = reinterpret_cast<CFDictionaryRef>(inData);


// restore the previously loaded audio file
	bool foundAudioFilePathString = false;
	bool failedToResolveAudioFile = false;
	dfx::UniqueCFType<CFStringRef> audioFileNameString;
	auto const fileReferencesDictionary = reinterpret_cast<CFDictionaryRef>(CFDictionaryGetValue(dict, CFSTR(kAUPresetExternalFileRefs)));
	if (fileReferencesDictionary)
	{
		auto const audioFileUrlString = reinterpret_cast<CFStringRef>(CFDictionaryGetValue(fileReferencesDictionary, kTurntablistPreset_AudioFileReferenceKey));
		if (audioFileUrlString)
		{
			dfx::UniqueCFType const audioFileUrl = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, audioFileUrlString, kCFURLPOSIXPathStyle, false);
			if (audioFileUrl)
			{
				FSRef audioFileRef;
				if (CFURLGetFSRef(audioFileUrl.get(), &audioFileRef))
				{
					loadAudioFile(audioFileRef);
					foundAudioFilePathString = true;
					failedToResolveAudioFile = false;
				}
				else
				{
					failedToResolveAudioFile = true;
					// we can't get the proper LaunchServices "display name" if the file does not exist, so do this instead
					audioFileNameString = CFURLCopyLastPathComponent(audioFileUrl.get());
				}
			}
		}
	}

	// if resolving the audio file from the stored file path failed, then try resolving from the stored file alias
	if (!foundAudioFilePathString)
	{
		auto const aliasCFData = reinterpret_cast<CFDataRef>(CFDictionaryGetValue(dict, kTurntablistPreset_AudioFileAliasKey));
		if (aliasCFData)
		{
			if (CFGetTypeID(aliasCFData) == CFDataGetTypeID())
			{
				auto const aliasDataSize = CFDataGetLength(aliasCFData);
				auto const aliasData = CFDataGetBytePtr(aliasCFData);
				if (aliasData && (aliasDataSize > 0))
				{
					AliasHandle aliasHandle = nullptr;
					auto aliasError = PtrToHand(aliasData, (Handle*)(&aliasHandle), aliasDataSize);
					if ((aliasError == noErr) && aliasHandle)
					{
						FSRef audioFileRef;
						Boolean wasChanged;
						aliasError = FSResolveAlias(nullptr, aliasHandle, &audioFileRef, &wasChanged);
						if (aliasError == noErr)
						{
							loadAudioFile(audioFileRef);
							failedToResolveAudioFile = false;
						}
						else
						{
							failedToResolveAudioFile = true;
							// if we haven't gotten it already...
							if (!audioFileNameString)
							{
								HFSUniStr255 fileNameUniString;
								auto const aliasStatus = FSCopyAliasInfo(aliasHandle, &fileNameUniString, nullptr, nullptr, nullptr, nullptr);
								if (aliasStatus == noErr)
								{
									audioFileNameString = CFStringCreateWithCharacters(kCFAllocatorDefault, fileNameUniString.unicode, fileNameUniString.length);
								}
							}
						}
						DisposeHandle((Handle)aliasHandle);
					}
				}
			}
		}
	}

	if (failedToResolveAudioFile)
	{
		PostNotification_AudioFileNotFound(audioFileNameString.get());
	}


// restore the MIDI CC -> parameter assignments
	getsettings().restoreMidiAssignmentsFromDictionary(dict);

	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus Turntablist::PostNotification_AudioFileNotFound(CFStringRef inFileName)
{
	if (!inFileName)
	{
		inFileName = CFSTR("");
	}

	auto const pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));

	dfx::UniqueCFType const titleString_base = CFCopyLocalizedStringFromTableInBundle(CFSTR("%@:  Audio file not found."), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("the title of the notification dialog when an audio file referenced in stored session data cannot be found"));
	auto const identifyingNameString = mContextName ? mContextName : CFSTR(PLUGIN_NAME_STRING);
	dfx::UniqueCFType const titleString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr, titleString_base.get(), identifyingNameString);

	dfx::UniqueCFType const messageString_base = CFCopyLocalizedStringFromTableInBundle(CFSTR("The previously used file \"%@\" could not be found."), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("the detailed message of the notification dialog when an audio file referenced in stored session data cannot be found"));
	dfx::UniqueCFType const messageString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr, messageString_base.get(), inFileName);

	OSStatus status = CFUserNotificationDisplayNotice(0.0, kCFUserNotificationPlainAlertLevel, nullptr, nullptr, nullptr, titleString.get(), messageString.get(), nullptr);

	return status;
}

//-----------------------------------------------------------------------------
OSStatus Turntablist::GetPropertyInfo(AudioUnitPropertyID inPropertyID, 
									  AudioUnitScope inScope, AudioUnitElement inElement, 
									  UInt32& outDataSize, Boolean& outWritable)
{
	OSStatus status = noErr;

	switch (inPropertyID)
	{
		case kTurntablistProperty_Play:
			outDataSize = sizeof(Boolean);
			outWritable = true;
			break;

		case kTurntablistProperty_AudioFile:
			if (FSIsFSRefValid(&m_fsAudioFile))
			{
				AliasHandle alias = nullptr;
				Size aliasSize = 0;
				status = createAudioFileAlias(&alias, &aliasSize);
				if (status == noErr)
				{
					outDataSize = aliasSize;
					outWritable = true;
					DisposeHandle((Handle)alias);
				}
			}
			else
			{
				status = errFSBadFSRef;
			}
			break;

		default:
			status = DfxPlugin::GetPropertyInfo(inPropertyID, inScope, inElement, outDataSize, outWritable);
			break;
	}

	return status;
}

//-----------------------------------------------------------------------------
OSStatus Turntablist::GetProperty(AudioUnitPropertyID inPropertyID, 
								  AudioUnitScope inScope, AudioUnitElement inElement, 
								  void* outData)
{
	OSStatus status = noErr;

	switch (inPropertyID)
	{
		case kTurntablistProperty_Play:
			*static_cast<Boolean*>(outData) = m_bPlay;
			break;

		case kTurntablistProperty_AudioFile:
			if (FSIsFSRefValid(&m_fsAudioFile))
			{
				AliasHandle alias = nullptr;
				Size aliasSize = 0;
				status = createAudioFileAlias(&alias, &aliasSize);
				if (status == noErr)
				{
					memcpy(outData, *alias, aliasSize);
					DisposeHandle((Handle)alias);
				}
			}
			else
				status = errFSBadFSRef;
			break;

		default:
			status = DfxPlugin::GetProperty(inPropertyID, inScope, inElement, outData);
			break;
	}

	return status;
}

//-----------------------------------------------------------------------------
OSStatus Turntablist::SetProperty(AudioUnitPropertyID inPropertyID, 
								  AudioUnitScope inScope, AudioUnitElement inElement, 
								  void const* inData, UInt32 inDataSize)
{
	OSStatus status = noErr;

	switch (inPropertyID)
	{
		case kTurntablistProperty_Play:
			m_bPlay = *static_cast<Boolean const*>(inData);
			playNote(m_bPlay);
			break;

		case kTurntablistProperty_AudioFile:
			{
				AliasHandle alias = nullptr;
				status = PtrToHand(inData, (Handle*)(&alias), inDataSize);
				if ((status == noErr) && alias)
				{
					status = resolveAudioFileAlias(alias);
					DisposeHandle((Handle)alias);
				}
				else
				{
					status = memFullErr;
				}
			}
			break;

		default:
			status = DfxPlugin::SetProperty(inPropertyID, inScope, inElement, inData, inDataSize);
			break;
	}

	return status;
}

//-----------------------------------------------------------------------------------------
OSStatus Turntablist::GetParameterInfo(AudioUnitScope inScope, 
									   AudioUnitParameterID inParameterID, 
									   AudioUnitParameterInfo& outParameterInfo)
{
	auto const status = DfxPlugin::GetParameterInfo(inScope, inParameterID, outParameterInfo);
	if (status != noErr)
	{
		return status;
	}

	switch (inParameterID)
	{
		case kParam_ScratchAmount:
		case kParam_ScratchMode:
		case kParam_ScratchSpeed_scrub:
		case kParam_ScratchSpeed_spin:
			HasClump(outParameterInfo, kParamGroup_Scratching);
			break;

		case kParam_Power:
		case kParam_SpinUpSpeed:
		case kParam_SpinDownSpeed:
		case kParam_NotePowerTrack:
			HasClump(outParameterInfo, kParamGroup_Power);
			break;

		case kParam_PitchShift:
		case kParam_PitchRange:
		case kParam_KeyTracking:
		case kParam_RootKey:
			HasClump(outParameterInfo, kParamGroup_Pitch);
			break;

		case kParam_Loop:
		case kParam_Direction:
		case kParam_NoteMode:
		case kParam_PlayTrigger:
			HasClump(outParameterInfo, kParamGroup_Playback);
			break;

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		case kParam_Mute:
		case kParam_Volume:
			HasClump(outParameterInfo, kParamGroup_Output);
			break;
#endif

		default:
			break;
	}

	return status;
}

//-----------------------------------------------------------------------------------------
OSStatus Turntablist::CopyClumpName(AudioUnitScope inScope, UInt32 inClumpID, 
									UInt32 inDesiredNameLength, CFStringRef* outClumpName)
{
	if (inClumpID < kParamGroup_BaseID)
	{
		return TARGET_API_BASE_CLASS::CopyClumpName(inScope, inClumpID, inDesiredNameLength, outClumpName);
	}
	if (inScope != kAudioUnitScope_Global)
	{
		return kAudioUnitErr_InvalidScope;
	}
	
	auto const pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	switch (inClumpID)
	{
		case kParamGroup_Scratching:
			*outClumpName = CFCopyLocalizedStringFromTableInBundle(CFSTR("scratching"), CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter clump name"));
			return noErr;
		case kParamGroup_Playback:
			*outClumpName = CFCopyLocalizedStringFromTableInBundle(CFSTR("audio sample playback"), CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter clump name"));
			return noErr;
		case kParamGroup_Power:
			*outClumpName = CFCopyLocalizedStringFromTableInBundle(CFSTR("turntable power"), CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter clump name"));
			return noErr;
		case kParamGroup_Pitch:
			*outClumpName = CFCopyLocalizedStringFromTableInBundle(CFSTR("pitch"), CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter clump name"));
			return noErr;
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		case kParamGroup_Output:
			*outClumpName = CFCopyLocalizedStringFromTableInBundle(CFSTR("audio output"), CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter clump name"));
			return noErr;
#endif
		default:
			return kAudioUnitErr_InvalidPropertyValue;
	}
}



//-----------------------------------------------------------------------------------------
void Turntablist::setPlay(bool inPlayState, bool inShouldSendNotification)
{
	if (std::exchange(m_bPlay, inPlayState) != inPlayState)
	{
		PropertyChanged(kTurntablistProperty_Play, kAudioUnitScope_Global, 0);
	}
}

//-----------------------------------------------------------------------------
OSStatus Turntablist::createAudioFileAlias(AliasHandle* outAlias, Size* outDataSize)
{
	if (!outAlias)
	{
		return paramErr;
	}

	auto const error = FSNewAlias(nullptr, &m_fsAudioFile, outAlias);
	if (error != noErr)
	{
		return error;
	}
	if (*outAlias == nullptr)
	{
		return nilHandleErr;
	}

	if (outDataSize)
	{
		*outDataSize = GetAliasSize(*outAlias);
	}

	return error;
}

//-----------------------------------------------------------------------------
OSStatus Turntablist::resolveAudioFileAlias(AliasHandle const inAlias)
{
	FSRef audioFileRef;
	Boolean wasChanged;
	auto status = FSResolveAlias(nullptr, inAlias, &audioFileRef, &wasChanged);
	if (status == noErr)
	{
		status = loadAudioFile(audioFileRef);
	}

	return status;
}



#pragma mark -
#pragma mark audio processing

//-----------------------------------------------------------------------------------------
OSStatus Turntablist::loadAudioFile(FSRef const& inFileRef)
{
// ExtAudioFile
#ifndef USE_LIBSNDFILE
	ExtAudioFileRef audioFileRef = nullptr;
	auto status = ExtAudioFileOpen(&inFileRef, &audioFileRef);
	if (status != noErr)
	{
		return status;
	}

	SInt64 audioFileNumFrames = 0;
	UInt32 dataSize = sizeof(audioFileNumFrames);
	status = ExtAudioFileGetProperty(audioFileRef, kExtAudioFileProperty_FileLengthFrames, &dataSize, &audioFileNumFrames);
	if (status != noErr)
	{
		return status;
	}

	AudioStreamBasicDescription audioFileStreamFormat;
	dataSize = sizeof(audioFileStreamFormat);
	status = ExtAudioFileGetProperty(audioFileRef, kExtAudioFileProperty_FileDataFormat, &dataSize, &audioFileStreamFormat);
	if (status != noErr)
	{
		return status;
	}

	constexpr bool audioInterleaved = false;
	CAStreamBasicDescription const clientStreamFormat(audioFileStreamFormat.mSampleRate, audioFileStreamFormat.mChannelsPerFrame, 
													  CAStreamBasicDescription::kPCMFormatFloat32, audioInterleaved);
	status = ExtAudioFileSetProperty(audioFileRef, kExtAudioFileProperty_ClientDataFormat, sizeof(clientStreamFormat), &clientStreamFormat);
	if (status != noErr)
	{
		return status;
	}

	std::unique_lock guard(m_AudioFileLock);

	m_bAudioFileHasBeenLoaded = false;	// XXX cuz we're about to possibly re-allocate the audio buffer and invalidate what might already be there

	m_auBufferList.Allocate(clientStreamFormat, static_cast<UInt32>(audioFileNumFrames));
	auto& abl = m_auBufferList.PrepareBuffer(clientStreamFormat, static_cast<UInt32>(audioFileNumFrames));
	auto audioFileNumFrames_temp = static_cast<UInt32>(audioFileNumFrames);
	status = ExtAudioFileRead(audioFileRef, &audioFileNumFrames_temp, &abl);
	if (status != noErr)
	{
		m_auBufferList.Deallocate();
		return status;
	}
	if (audioFileNumFrames_temp != static_cast<UInt32>(audioFileNumFrames))  // XXX do something?
	{
		// XXX error?
		fprintf(stderr, PLUGIN_NAME_STRING ":  audio data size mismatch!\nsize requested: %ld, size read: %u\n\n", static_cast<long>(audioFileNumFrames), static_cast<unsigned int>(audioFileNumFrames_temp));
	}

	status = ExtAudioFileDispose(audioFileRef);

	m_nNumChannels = clientStreamFormat.mChannelsPerFrame;
	m_nSampleRate = clientStreamFormat.mSampleRate;
	m_nNumSamples = static_cast<int>(audioFileNumFrames);

	m_fPlaySampleRate = static_cast<double>(m_nSampleRate);
	m_fSampleRate = static_cast<double>(m_nSampleRate);
	calculateSpinSpeeds();
	m_fPosition = 0.0;
	m_fPosOffset = 0.0;
	m_fNumSamples = static_cast<double>(m_nNumSamples);



// libsndfile
#else
	UInt8 file[2048];
	memset(file, 0, sizeof(file));
	OSStatus status = FSRefMakePath(&inFileRef, file, sizeof(file));
	if (status != noErr)
	{
		return status;
	}
//fprintf(stderr, PLUGIN_NAME_STRING " audio file:  %s\n", file);

	SF_INFO sfInfo;
	memset(&sfInfo, 0, sizeof(SF_INFO));
	SNDFILE* const sndFile = sf_open((const char*)file, SFM_READ, &sfInfo);

	if (!sndFile)
	{
		// print error
		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		sf_error_str(sndFile, buffer, sizeof(buffer) - 1);
		fprintf(stderr, "\n" PLUGIN_NAME_STRING " could not open the audio file:  %s\nlibsndfile error message:  %s\n", file, buffer);
		return sf_error(sndFile);
	}

#if 0
	fprintf(stderr, "**** SF_INFO dump for:  %s\n", file);
	fprintf(stderr, "     samplerate:  %d\n", sfInfo.samplerate);
	fprintf(stderr, "     frames:  %d (0x%08x)\n", sfInfo.frames, sfInfo.frames);
	fprintf(stderr, "     channels:  %d\n", sfInfo.channels);
	fprintf(stderr, "     format:  %d\n", sfInfo.format);
	fprintf(stderr, "     sections:  %d\n", sfInfo.sections);
	fprintf(stderr, "     seekable:  %d\n", sfInfo.seekable);
#endif

	sf_command(sndFile, SFC_SET_NORM_FLOAT, nullptr, SF_TRUE);

	std::unique_lock guard(m_AudioFileLock);

	m_nNumChannels = sfInfo.channels;
	m_nSampleRate = sfInfo.samplerate;
	m_nNumSamples = static_cast<int>(sfInfo.frames);

	m_fPlaySampleRate = static_cast<double>(m_nSampleRate);
	m_fSampleRate = static_cast<double>(m_nSampleRate);
	calculateSpinSpeeds();
	m_fPosition = 0.0;
	m_fPosOffset = 0.0;
	m_fNumSamples = static_cast<double>(m_nNumSamples);

	m_fLeft = nullptr;
	m_fRight = nullptr;

	m_fBuffer.assign(m_nNumChannels * m_nNumSamples, 0.0f);

	m_fLeft = m_fBuffer.data();
	m_fRight = m_fBuffer.data() + ((m_nNumChannels == 1) ? 0 : m_nNumSamples);

	// do file loading here!!
	sf_count_t const sizein = sfInfo.frames * sfInfo.channels;
	sf_count_t sizeout = sf_read_float(sndFile, m_fBuffer.data(), sizein);
	if (sizeout != sizein)
	{
		// XXX error?
		fprintf(stderr, PLUGIN_NAME_STRING ":  audio data size mismatch!\nsize-in: %ld, size-out: %ld\n\n", static_cast<long>(sizein), static_cast<long>(sizeout));
	}

	if (m_nNumChannels >= 2)
	{
		std::vector<float> tempBuffer(m_nNumSamples, 0.0f);

		// do swaps
		for (size_t z = 0; z < tempBuffer.size(); z++)
		{
			tempBuffer[z] = m_fBuffer[(z * m_nNumChannels) + 1];	// copy channel 2 into buffer
			m_fBuffer[z] = m_fBuffer[z * m_nNumChannels];	// handle channel 1
		}

		for (size_t z = 0; z < tempBuffer.size(); z++)
		{
			m_fBuffer[m_nNumSamples + z] = tempBuffer[z];	// make channel 2
		}
	}

	// end file loading here!!

	if (sf_close(sndFile) != 0)
	{
		// error closing
	}
#endif


	processPitch();	// set up stuff

	// ready to play
	m_bAudioFileHasBeenLoaded = true;
	guard.unlock();

	m_fsAudioFile = inFileRef;  // XXX TODO: this object needs its own serialized lock access as well
	PropertyChanged(kTurntablistProperty_AudioFile, kAudioUnitScope_Global, 0);

	return noErr;
}

//-----------------------------------------------------------------------------------------
void Turntablist::calculateSpinSpeeds()
{
//	m_fUsedSpinUpSpeed = (((std::exp(10.0 * m_fSpinUpSpeed) - 1.0) / (std::exp(10.0) - 1.0)) * m_fSampleRate) / static_cast<double>(m_nPowerIntervalEnd);
//	m_fUsedSpinDownSpeed = (((std::exp(10.0 * m_fSpinDownSpeed) - 1.0) / (std::exp(10.0) - 1.0)) * m_fSampleRate) / static_cast<double>(m_nPowerIntervalEnd);
	m_fUsedSpinUpSpeed = m_fSpinUpSpeed * m_fSampleRate / static_cast<double>(m_nPowerIntervalEnd);
	m_fUsedSpinDownSpeed = m_fSpinDownSpeed * m_fSampleRate / static_cast<double>(m_nPowerIntervalEnd);
}

//-----------------------------------------------------------------------------------------
void Turntablist::processaudio(float const* const* /*inAudio*/, float* const* outAudio, unsigned long inNumFrames)
{
	long eventFrame = -1; // -1 = no events
	auto const numEvents = getmidistate().getBlockEventCount();
	long currEvent = 0;

	unsigned long numOutputs = getnumoutputs();
//	float (*interpolateHermiteFunctionPtr)(float *, double, long) = m_bLoop ? dfx::math::InterpolateHermite : dfx::math::InterpolateHermite_NoWrap;


	if (numEvents == 0)
	{
		eventFrame = -1;
	}
	else
	{
		eventFrame = getmidistate().getBlockEvent(currEvent).mOffsetFrames;
	}
	for (long currFrame = 0; currFrame < (signed)inNumFrames; currFrame++)
	{
		//
		// process MIDI events if any
		//
		while (currFrame == eventFrame)
		{
			processMidiEvent(currEvent);
			currEvent++;	// check next event

			if (currEvent >= numEvents)
				eventFrame = -1;	// no more events
		}
		
		if (m_bScratching)  // handle scratching
		{
			m_nScratchInterval++;
			if (m_nScratchInterval > m_nScratchIntervalEnd)
			{
				if (m_nScratchMode == kScratchMode_Spin)
					processScratch();
				else
					processScratchStop();
			}
			else
			{
				if (m_nScratchMode == kScratchMode_Scrub)
				{
					// nudge samplerate
					m_fPlaySampleRate += m_fTinyScratchAdjust;
					// speed mode
					if (m_fTinyScratchAdjust > 0.0)	// positive
					{
						if (m_fPlaySampleRate > m_fDesiredScratchRate2)
							m_fPlaySampleRate = m_fDesiredScratchRate2;
					}
					else	// negative
					{
						if (m_fPlaySampleRate < m_fDesiredScratchRate2)
							m_fPlaySampleRate = m_fDesiredScratchRate2;
					}					
				}				
			}
		}
		else // not scratching so just handle power
		{
			if (!m_bPower)	// power off - spin down
			{
				if (m_fPlaySampleRate > 0.0)	// too high, spin down
				{
					m_fPlaySampleRate -= m_fUsedSpinDownSpeed;	// adjust
					if (m_fPlaySampleRate < 0.0)	// too low so we past it
						m_fPlaySampleRate = 0.0;	// fix it
				}
			}
			else // power on - spin up
			{
				if (m_fPlaySampleRate < m_fDesiredPitch) // too low so bring up
				{
					m_fPlaySampleRate += m_fUsedSpinUpSpeed; // adjust
					if (m_fPlaySampleRate > m_fDesiredPitch) // too high so we past it
						m_fPlaySampleRate = m_fDesiredPitch; // fix it
				}
				else if (m_fPlaySampleRate > m_fDesiredPitch) // too high so bring down
				{
					m_fPlaySampleRate -= m_fUsedSpinUpSpeed; // adjust
					if (m_fPlaySampleRate < m_fDesiredPitch) // too low so we past it
						m_fPlaySampleRate = m_fDesiredPitch; // fix it
				}
			}
		}

		// handle rest of processing here
		if (m_bNoteIsOn)
		{
			// adjust gnPosition for new sample rate

			// set pos offset based on current sample rate

			// safety check
			if (m_fPlaySampleRate <= 0.0)
			{
				m_fPlaySampleRate = 0.0;
				if (m_bNotePowerTrack)
				{
					if (!m_bScratching)
					{
						stopNote(true);
						if (m_nNoteMode == kNoteMode_Reset)
							m_fPosition = 0.0;
					}
				}
			}

			m_fPosOffset = m_fPlaySampleRate / getsamplerate();   //m_fPlaySampleRate / m_fSampleRate;

			std::unique_lock guard(m_AudioFileLock, std::defer_lock);
			if (m_bAudioFileHasBeenLoaded)
			{
				guard.try_lock();
			}
			if (guard.owns_lock())
			{
				if (m_bNoteIsOn)
				{

					if (!m_bPlayForward) // if play direction = reverse
					{
						if (m_fPlaySampleRate != 0.0)
						{
							m_fPosition -= m_fPosOffset;
							while (m_fPosition < 0.0)	// was if
							{
								if (!m_bLoop)	// off
								{
									if (m_bPlayedReverse)
									{
										stopNote(true);
										m_fPosition = 0.0;
									}
									else
									{
										m_fPosition += m_fNumSamples; // - 1;
										m_bPlayedReverse = true;
									}									
								}
								else
									m_fPosition += m_fNumSamples; // - 1;
							}
						}
					}  // if (!bPlayForward)

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
					if (!m_bMute)   // if audio on
					{		
#endif
						if (m_fPlaySampleRate == 0.0)
						{
							for (unsigned long ch=0; ch < numOutputs; ch++)
								outAudio[ch][currFrame] = 0.0f;
						}
						else
						{
//#define NO_INTERPOLATION
//#define LINEAR_INTERPOLATION
#define CUBIC_INTERPOLATION

							for (unsigned long ch=0; ch < numOutputs; ch++)
							{
							#ifdef USE_LIBSNDFILE
								float * output = (ch == 0) ? m_fLeft : m_fRight;
							#else
								AudioBufferList & abl = m_auBufferList.GetBufferList();
								unsigned long ablChannel = ch;
								if (ch >= abl.mNumberBuffers)
								{
									ablChannel = abl.mNumberBuffers - 1;
									// XXX only do the channel remapping for mono->stereo upmixing special case (?)
									if (ch > 1)
										break;
								}
								float * output = (float*) (abl.mBuffers[ablChannel].mData);
							#endif

#ifdef NO_INTERPOLATION
								auto const outval = output[static_cast<size_t>(m_fPosition)];
#endif  // NO_INTERPOLATION

#ifdef LINEAR_INTERPOLATION						
								float const floating_part = m_fPosition - static_cast<double>(static_cast<long>(m_fPosition));
								long const big_part1 = static_cast<long>(m_fPosition);
								long big_part2 = big_part1 + 1;
								if (big_part2 > m_nNumSamples)
									big_part2 = 0;
								float const outval = (floating_part * output[big_part1]) + ((1.0f - floating_part) * output[big_part2]);
#endif  // LINEAR_INTERPOLATION

#ifdef CUBIC_INTERPOLATION
								float outval;
							#if 0
								// XXX is this a silly optimization to avoid another branch?
								// XXX can this even work for an inline function?
								outval = interpolateHermiteFunctionPtr(output, m_fPosition, m_nNumSamples);
							#else
								if (m_bLoop)
								{
									outval = dfx::math::InterpolateHermite(output, m_fPosition, m_nNumSamples);
								}
								else
								{
									outval = dfx::math::InterpolateHermite_NoWrap(output, m_fPosition, m_nNumSamples);
								}
							#endif
#endif  // CUBIC_INTERPOLATION

								outAudio[ch][currFrame] = outval * m_fNoteVolume;
							}
						}
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
					}  // if (!m_bMute)
					else
					{
						for (unsigned long ch=0; ch < numOutputs; ch++)
							outAudio[ch][currFrame] = 0.0f;
					}
#endif

					
					if (m_bPlayForward)	// if play direction = forward
					{
						m_bPlayedReverse = false;
						if (m_fPlaySampleRate != 0.0)
						{
							m_fPosition += m_fPosOffset;

							while (m_fPosition >= m_fNumSamples)
							{
								m_fPosition -= m_fNumSamples;
								if (!m_bLoop) // off
									stopNote(true);
							}
						}
					}  // if (bPlayForward)

				}  // if (bNoteIsOn)
				else
				{
					for (unsigned long ch=0; ch < numOutputs; ch++)
						outAudio[ch][currFrame] = 0.0f;
				}
			}  // if (owns_lock)
			else
			{
				for (unsigned long ch=0; ch < numOutputs; ch++)
					outAudio[ch][currFrame] = 0.0f;
			}
		}  // if (bNoteIsOn)
		else
		{
			for (unsigned long ch=0; ch < numOutputs; ch++)
				outAudio[ch][currFrame] = 0.0f;
		}
	}  // (currFrame < inNumFrames)
}



#pragma mark -
#pragma mark scratch processing

//-----------------------------------------------------------------------------------------
void Turntablist::processScratchStop()
{
	m_nScratchDir = kScratchDirection_Forward;
	m_fPlaySampleRate = 0.0;
	if (!m_bScratchStop)
	{
		m_bScratchStop = true;
//		m_nScratchInterval = m_nScratchIntervalEnd / 2;
	}
	else
	{
		// TO DO:
		// 1. set the position to where it should be to adjust for errors

		m_fPlaySampleRate = 0.0;
		m_fDesiredScratchRate = 0.0;
		m_fDesiredScratchRate2 = 0.0;
		m_fTinyScratchAdjust = 0.0;
		m_nScratchInterval = 0;
	}

	processDirection();
}

//-----------------------------------------------------------------------------------------
void Turntablist::processScratch(bool inSetParameter)
{
	double fIntervalScaler = 0.0;
	
	if (m_bPitchBendSet)
	{
/*
		// set scratch amount to scaled pitchbend
		if (inSetParameter)
		{
			setparameter_f(kParam_ScratchAmount, m_fPitchBend);
			// XXX post notification?  not that this ever gets called with inSetParameter true...
			m_fScratchAmount = m_fPitchBend;
		}
		else
			m_fScratchAmount = m_fPitchBend;
*/
		m_bScratchAmountSet = true;

		m_bPitchBendSet = false;
	}


	if (m_bScratching)  // scratching
	{
		if (m_nScratchMode == kScratchMode_Spin)
		{
			if (m_fScratchAmount >= k_fScratchAmountMiddlePoint)
				m_nScratchDir = kScratchDirection_Forward;
			else
				m_nScratchDir = kScratchDirection_Backward;
		}

	// todo:
	// handle scratching like power
	// scratching will set target sample rate and system will catch up based on scratch speed parameter
	// sort of like spin up/down speed param..

	// if curr rate below target rate then add samplespeed2
	// it gone over then set to target rate
	// if curr rate above target rate then sub samplespeed2
	// if gone below then set to target rate


		// m_fScratchSpeed_spin is the hand size
		// set target sample rate
		m_fDesiredScratchRate = fabs(m_fScratchAmount * m_fScratchSpeed_spin * m_fBasePitch);
		
		if (m_nScratchMode == kScratchMode_Spin)	// mode 2
		{
			m_fPlaySampleRate = m_fDesiredScratchRate;
			m_bScratchStop = false;
			m_nScratchInterval = 0;
		}
		else	// mode 1
		{
		//	int oldtime = timeGetTime();  //time in nanoseconds

			//	NEWEST MODE 2
			// TO DO:
			// make position based
			// use torque to set the spin down time
			// and make the torque speed be samplerate/spindown time

			if (m_nScratchSubMode == 0)
			{
// POSITION BASED:
				if (m_bScratchAmountSet)
				{
					m_bScratchStop = false;

					fIntervalScaler = (static_cast<double>(m_nScratchInterval) / static_cast<double>(m_nScratchIntervalEnd)) + 1.0;

					m_fDesiredPosition = m_fScratchCenter + (contractparametervalue(kParam_ScratchAmount, m_fScratchAmount) * m_fScratchSpeed_scrub * m_fSampleRate);

					double fDesiredDelta;
					
					if (m_nScratchInterval == 0)
					{
						m_fPosition = m_fDesiredPosition;
						m_fTinyScratchAdjust = 0.0;
					}
					else
					{
						fDesiredDelta = (m_fDesiredPosition - m_fPosition); //m_nScratchInterval;
						m_fTinyScratchAdjust = (fDesiredDelta - m_fPlaySampleRate) / m_fSampleRate;
					}








					// do something with desireddelta and scratchinterval

					// figure out direction
					double fDiff = contractparametervalue(kParam_ScratchAmount, m_fScratchAmount) - contractparametervalue(kParam_ScratchAmount, m_fLastScratchAmount);

					if (fDiff < 0.0)
					{
						fDiff = -fDiff;
						m_nScratchDir = kScratchDirection_Backward;
					}
					else
						m_nScratchDir = kScratchDirection_Forward;

					m_fDesiredScratchRate2 = m_fSampleRate * m_fScratchSpeed_scrub * static_cast<double>(m_nScratchInterval);

					// figure out destination position and current position
					// figure out distance between the two
					// samples to cover = fDiff * m_nNumSamples
					// need algorithm to get the acceleration needed to cover the distance given current speed
					// calculate the sample rate needed to cover that distance in that amount of time (interval end)
					// figure out desiredscratchrate2
					// m_nScratchIntervalEnd/m_nSampleRate = m_nPosDiff/x
					//m_fDesiredScratchRate2 = (m_nSampleRate/m_nScratchIntervalEnd)*m_nPosDiff;
					//if fDiff == 1.0 then it means ur moving thru the entire sample in one quick move
					//
					//
					// desired rate = distance / time
					// distance is samples to cover, time is interval
					//accel = (desired rate - curr rate)/ time
					// accel is tiny scratch adjust

					// TO DO:
					// 1. figure out how to handle wrapping around the sample

					//new
					// add fScaler = (m_fScratchSpeed*7.0f)
					


					m_fDesiredScratchRate2 = (m_fDesiredScratchRate2 + m_fPlaySampleRate) * 0.5;

					m_bScratchAmountSet = false;

					m_fPrevDesiredPosition = m_fDesiredPosition;
				}
			}
			else
			{
// SPEED BASED:
				if (m_bScratchAmountSet)
				{
					m_bScratchStop = false;

					double fDiff = contractparametervalue(kParam_ScratchAmount, m_fScratchAmount) - contractparametervalue(kParam_ScratchAmount, m_fLastScratchAmount);

					if (fDiff < 0.0)
					{
						fDiff = -fDiff;
						m_nScratchDir = kScratchDirection_Backward;
					}
					else
					{
						m_nScratchDir = kScratchDirection_Forward;
					}


					// new
					fIntervalScaler = (static_cast<double>(m_nScratchInterval) / static_cast<double>(m_nScratchIntervalEnd)) + 1.0;

					m_fDesiredScratchRate2 = (fDiff * m_fSampleRate * m_fScratchSpeed_scrub * static_cast<double>(k_nScratchInterval)) / fIntervalScaler;

					m_fTinyScratchAdjust = (m_fDesiredScratchRate2 - m_fPlaySampleRate) / static_cast<double>(m_nScratchIntervalEnd); 


					m_fPlaySampleRate += m_fTinyScratchAdjust;
					m_bScratchAmountSet = false;
				}
			}

			m_fLastScratchAmount = m_fScratchAmount;
		}
		m_fScratchVolumeModifier = m_fPlaySampleRate / m_fSampleRate;
		if (m_fScratchVolumeModifier > 1.5f)
			m_fScratchVolumeModifier = 1.5f;

		if (!m_bScratching)
			m_bScratching = true;

		if (m_bScratchStop)
		{
			m_fPlaySampleRate = 0.0;
			m_fDesiredScratchRate = 0.0;
			m_fDesiredScratchRate2 = 0.0;
		}

		m_nScratchInterval = 0;


		processDirection();
	}
	else
	{
		m_nWasScratchingDir = m_nScratchDir;
		m_bScratching = false;
		m_bWasScratching = true;
		processDirection();
	}
}

//-----------------------------------------------------------------------------------------
void Turntablist::processPitch()
{
	auto const entryDesiredPitch = m_fDesiredPitch;
	if (m_bKeyTracking)
	{
		auto const note2play = m_nCurrentNote - m_nRootKey;
		m_fBasePitch = m_fSampleRate * std::pow(2.0, static_cast<double>(note2play) / 12.0);
	}
	else
	{
		m_fBasePitch = m_fSampleRate;
	}
	double fPitchAdjust = m_fPitchShift * m_fPitchRange;  // shift in semitones
	fPitchAdjust = std::pow(2.0, fPitchAdjust / 12.0);  // shift as a scalar of the playback rate
	m_fDesiredPitch = m_fBasePitch * fPitchAdjust;

	if (entryDesiredPitch == m_fPlaySampleRate)
	{
		m_fPlaySampleRate = m_fDesiredPitch;  // do instant adjustment
	}
}

//-----------------------------------------------------------------------------------------
void Turntablist::processDirection()
{
	m_bPlayForward = true;

	if (m_bScratching)
	{
		if (m_nScratchDir == kScratchDirection_Backward)
			m_bPlayForward = false;
	}
	else
	{
		if (m_bWasScratching)
		{
			if (m_nDirection == kScratchDirection_Backward)
				m_bPlayForward = false;
		}
		else
		{
			if (m_nDirection == kScratchDirection_Backward)
				m_bPlayForward = false;
		}
	}
}



#pragma mark -
#pragma mark MIDI processing

//-----------------------------------------------------------------------------------------
void Turntablist::processMidiEvent(long inCurrentEvent)
{
	auto const& event = getmidistate().getBlockEvent(inCurrentEvent);

	if (DfxMidi::isNote(event.mStatus))
	{
		auto const note = event.mByte1;
		auto const velocity = (event.mStatus == DfxMidi::kStatus_NoteOff) ? 0 : event.mByte2;
		noteOn(note, velocity, event.mOffsetFrames);  // handle note on or off
	}
	else if (event.mStatus == DfxMidi::kStatus_CC)
	{
		if (event.mByte1 == DfxMidi::kCC_AllNotesOff)
		{
			stopNote(true);
			if (m_nNoteMode == kNoteMode_Reset)
			{
				m_fPosition = 0.0;
			}
		}

#ifdef USE_MIDI_CC
		if ((event.mByte1 >= 64) && (event.mByte1 <= (64 + kNumParameters - 1)))
		{
			long const param = event.mByte1 - 64;
			long const new_data = fixMidiData(param, event.mByte2);
			float const value = static_cast<float>(new_data) * DfxMidi::kValueScalar;
			setparameter_f(param, value);
			postupdate_parameter(param);
		}
#endif
	}
#ifdef USE_MIDI_PITCH_BEND
	else if (event.mStatus == DfxMidi::kStatus_PitchBend)
	{
		// handle pitch bend here
		int const pitchBendValue = (event.mByte2 << 7) | event.mByte1;
		m_bPitchBendSet = true;

		if (pitchBendValue == DfxMidi::kPitchBendMidpointValue)
		{
			m_fPitchBend = k_fScratchAmountMiddlePoint;
		}
		else
		{
			m_fPitchBend = static_cast<double>(pitchBendValue) / static_cast<double>((DfxMidi::kPitchBendMidpointValue * 2) - 1);
			m_fPitchBend = std::clamp(m_fPitchBend, 0.0, 1.0);
			m_fPitchBend = expandparametervalue(kParam_ScratchAmount, m_fPitchBend);
		}

		setparameter_f(kParam_ScratchAmount, m_fPitchBend);
//		postupdate_parameter(kParam_ScratchAmount);	// XXX post notification? for some reason it makes the results sound completely different
		m_fScratchAmount = m_fPitchBend;
	}
#endif
}

//-----------------------------------------------------------------------------------------
void Turntablist::noteOn(int inNote, int inVelocity, unsigned long /*inOffsetFrames*/)
{
	auto const power_old = m_bPower;

	m_nCurrentNote = inNote;
	m_nCurrentVelocity = inVelocity;
	if (inVelocity == 0)
	{
		if (m_bNotePowerTrack)
		{
			setparameter_b(kParam_Power, false);
			m_bPower = false;
			m_bNoteIsOn = true;
			setPlay(false);
		}
		else
		{
			m_bNoteIsOn = false;
			setPlay(false);
			if (m_nNoteMode == kNoteMode_Reset)
			{
				m_fPosition = 0.0;
			}
			m_bPlayedReverse = false;
		}
	}
	else
	{
		m_bNoteIsOn = true;
		setPlay(true);

		if (m_nNoteMode == kNoteMode_Reset)
		{
			m_fPosition = 0.0;
		}
		// calculate note volume
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		m_fNoteVolume = m_fVolume * static_csat<float>(m_nCurrentVelocity) * DfxMidi::kValueScalar;
#else
		m_fNoteVolume = static_cast<float>(m_nCurrentVelocity) * DfxMidi::kValueScalar;
#endif
		//44100*(2^(0/12)) = C-3
		processPitch();

		if (m_bNotePowerTrack)
		{
			setparameter_b(kParam_Power, true);
			m_bPower = true;
		}
	}

	// XXX post notification yes?
	if (m_bPower != power_old)
	{
		postupdate_parameter(kParam_Power);
	}
}

//-----------------------------------------------------------------------------------------
void Turntablist::stopNote(bool inStopPlay)
{
	m_bNoteIsOn = false;
	m_bPlayedReverse = false;

	if (inStopPlay)
	{
		setPlay(false);
	}
}

//-----------------------------------------------------------------------------------------
void Turntablist::playNote(bool inValue)
{
	if (inValue)
	{
		// m_nCurrentNote - m_nRootKey = ?
		// - 64       - 0          = -64
		// 64          - 64         = 0;
		// 190           - 127        = 63;
		m_nCurrentNote = m_nRootKey;
		noteOn(m_nCurrentNote, 0x7F, 0);	// note on
	}
	else
	{
		noteOn(m_nCurrentNote, 0, 0);  // note off
	}
}

//-----------------------------------------------------------------------------------------
long Turntablist::fixMidiData(long inParameterID, char inValue)
{
	switch (inParameterID)
	{
		case kParam_Power:
		case kParam_NotePowerTrack:
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		case kParam_Mute:
#endif
		case kParam_PlayTrigger:
		case kParam_NoteMode:
		case kParam_Direction:
		case kParam_ScratchMode:
		case kParam_Loop:
		case kParam_KeyTracking:
			// <64 = 0ff, >=64 = 0n
			return (inValue < 64) ? 0 : 0x7F;
		default:
			break;
	}

	return inValue;
}
