/*-------------- by Marc Poirier  ][  January - March 2001 -------------*/

#ifndef __REZSYNTH_H
#include "rezsynth.hpp"
#endif


void RezSynth::processaudio(const float **in, float **out, unsigned long inNumFrames, bool replacing)
{
	unsigned long numChannels = getnumoutputs();
	long numFramesToProcess = (signed)inNumFrames, totalSampleFrames = (signed)inNumFrames;	// for dividing up the block accoring to events
	float SAMPLERATE = getsamplerate_f();
	float dryGain = sqrtf(1.0f - dryWetMix);	// the gain of the input audio in the dry/wet mix
	wetGain = sqrtf(dryWetMix);
	bool mixDryAndWet = (dryWetMix < 1.0f);


	// clear the output buffer because we accumulate output into it
	if (replacing)
	{
		for (unsigned long cha=0; cha < numChannels; cha++)
		{
			for (unsigned long samp=0; samp < inNumFrames; samp++)
				out[cha][samp] = 0.0f;
		}
	}


	// these are 2 values that are always needed during processCoefficients
	twoPiDivSR = PId*2.0 / (double)SAMPLERATE;
	nyquist = ((double)SAMPLERATE-bandwidth) / 2.0;	// adjusted for bandwidth to accomodate the filter's frequency range

	// counter for the number of MIDI events this block
	// start at -1 because the beginning stuff has to happen
	long eventcount = -1;
	long currentBlockPosition = 0;	// we are at sample 0

	// mix very quiet noise (-300 dB) into the input singal to hopefully avoid any denormal values
	float quietNoise = 1.0e-15f;
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		float *volatileIn = (float*) in[ch];
		for (long samplecount = 0; samplecount < totalSampleFrames; samplecount++)
		{
			volatileIn[samplecount] += quietNoise;
			quietNoise = -quietNoise;
		}
	}


	// now we're ready to start looking at MIDI messages & processing sound & such
	do
	{
		// check for an upcoming event & decrease this block chunk size accordingly 
		// if there won't be another event, go all the way to the end of the block
		if ( (eventcount+1) >= midistuff->numBlockEvents )
			numFramesToProcess = totalSampleFrames - currentBlockPosition;
		// else there will be & this chunk goes up to the next delta position
		else
			numFramesToProcess = midistuff->blockEvents[eventcount+1].delta - currentBlockPosition;

		// this means that 2 (or more) events occur simultaneously, 
		// so there's no need to do calculations during this round
		if (numFramesToProcess == 0)
		{
			eventcount++;
			checkForNewNote(eventcount, numChannels);	// & attend to related issues if necessary
			// take in the effects of the next event
			midistuff->heedEvents(eventcount, SAMPLERATE, pitchbendRange, attack, release, legato, velCurve, velInfluence);
			continue;
		}

		// test for whether or not all notes are off & unprocessed audio can be outputted
		bool noNotes = true;	// none yet for this chunk

		for (int notecount=0; notecount < NUM_NOTES; notecount++)
		{
			// only go into the output processing cycle if a note is happening
			if (midistuff->noteTable[notecount].velocity)
			{
				noNotes = false;	// we have a note

				double ampEvener;	// a scalar for balancing outputs from the 3 normalizing modes
				// do the smart gain control thing if the user says so
				if (wiseAmp)
					ampEvener = processAmpEvener(numBands, notecount);
				else
					ampEvener = 1.0;

				// most of the note values are liable to change during processFilterOuts,
				// so we back them up to allow multi-band repetition
				NoteTable noteTemp = midistuff->noteTable[notecount];

				// this is the resonator stuff
				processCoefficients(&numBands, notecount);

				// restore the note values before doing processFilterOuts for the left channel
				midistuff->noteTable[notecount] = noteTemp;
				//
				processFilterOuts(&(in[0][currentBlockPosition]), &(out[0][currentBlockPosition]), 
								numFramesToProcess, ampEvener, notecount, numBands, 
								prevOutValue[notecount], prevprevOutValue[notecount]);

				if (numChannels == 2)
				{
					// restore the note values before doing processFilterOuts for the right channel
					midistuff->noteTable[notecount] = noteTemp;
					//
					processFilterOuts(&(in[1][currentBlockPosition]), &(out[1][currentBlockPosition]), 
									numFramesToProcess, ampEvener, notecount, numBands, 
									prevOut2Value[notecount], prevprevOut2Value[notecount]);
				}
			}
		}	// end of notes loop

		// we had notes this chunk, but the unaffected processing hasn't faded out, so change its state to fade-out
		if ( (!noNotes) && (unaffectedState == unFlat) )
			unaffectedState = unFadeOut;

		// we can output unprocessed audio if no notes happened during this block chunk
		// or if the unaffected fade-out still needs to be finished
		if ( (noNotes) || (unaffectedState == unFadeOut) )
		{
			int tempUnState = unaffectedState;
			int tempUnSamples = unaffectedFadeSamples;
			processUnaffected(&(in[0][currentBlockPosition]), &(out[0][currentBlockPosition]), numFramesToProcess);
			if (numChannels == 2)
			{
				unaffectedState = tempUnState;
				unaffectedFadeSamples = tempUnSamples;
				processUnaffected(&(in[1][currentBlockPosition]), &(out[1][currentBlockPosition]), numFramesToProcess);
			}
		}

		eventcount++;
		// don't do the event processing below if there are no more events
		if (eventcount >= midistuff->numBlockEvents)
			continue;

		// jump our position value forward
		currentBlockPosition = midistuff->blockEvents[eventcount].delta;

		checkForNewNote(eventcount, numChannels);	// & attend to related issues if necessary
		// take in the effects of the next event
		midistuff->heedEvents(eventcount, SAMPLERATE, pitchbendRange, attack, release, legato, velCurve, velInfluence);

	} while (eventcount < midistuff->numBlockEvents);

	// processEvents() is only called when new VstEvents are sent,
	// so zeroing numBlockEvents right here is critical
	midistuff->numBlockEvents = 0;

	// mix in the dry input (only if there is supposed to be some dry; let's not waste calculations)
	if (mixDryAndWet)
	{
		for (unsigned long ch=0; ch < numChannels; ch++)
		{
			for (long samplecount=0; samplecount < totalSampleFrames; samplecount++)
				out[ch][samplecount] += in[ch][samplecount] * dryGain;
		}
	}
}

