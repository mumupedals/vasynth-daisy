

#include "daisy_seed.h"
#include "daisysp.h"

#include "main.h"
#include "vasynth.h"

using namespace daisy;
using namespace daisysp;

// globals
DaisySeed hardware;
MidiUsbHandler midi;
float sysSampleRate;
float sysCallbackRate;
extern uint8_t preset_max;
extern VASynthSetting preset_setting[PRESET_MAX];

uint8_t param_;
float pitch_bend = 1.0f;
float master_tune = 0.0f;

// sound
VASynth vasynth;
uint8_t gPlay = PLAY_ON;

// fx
DelayLine<float, DELAY_MAX> DSY_SDRAM_BSS delay_;

// sequencer
void SequencerPlay(uint16_t);
void SequencerRecord(uint8_t, uint8_t);
void SeqTimer_Config(void);
void writeSram(uint32_t, uint8_t);
uint8_t readSram(uint32_t);
void writeSramWord(uint32_t, uint16_t);
uint16_t readSramWord(uint32_t);

uint16_t seqclock = 0, seqtime = 0, seqmode = 0, seqmsg;
uint16_t seqnote, seqvelocity;
uint32_t seqmem = 0x00010000;

// audio callback

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{	
	float voice_left, voice_right;
	
	for (size_t n = 0; n < size; n += 2)
	{	
		if (gPlay == PLAY_ON)
		{			
			vasynth.Process(&voice_left, &voice_right);
			
			out[n] = voice_left + in[n];
			out[n + 1] = voice_right + in[n + 1];
		} 
		else 
		{
			out[n] = 0;
			out[n + 1] = 0;		
		}		
	}
}


// midi handler
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            if ((vasynth.midi_channel_ == MIDI_CHANNEL_ALL) || (p.channel == vasynth.midi_channel_))
			{
				if(seqmode == 1)
				{
					SequencerRecord((p.note | 0x80), p.velocity);
				}
				vasynth.NoteOn(p.note, p.velocity);
				hardware.SetLed(true);
			}
	        break;
        }
        case NoteOff:
        {
            NoteOnEvent p = m.AsNoteOn();
            if ((vasynth.midi_channel_ == MIDI_CHANNEL_ALL) || (p.channel == vasynth.midi_channel_))
			{
				if(seqmode == 1)
				{
					SequencerRecord(p.note, 0);
				}
				vasynth.NoteOff(p.note);
				hardware.SetLed(false);
			}			
	        break;
        } 
		case PitchBend:
        {
            PitchBendEvent p = m.AsPitchBend();
            if ((vasynth.midi_channel_ == MIDI_CHANNEL_ALL) || (p.channel == vasynth.midi_channel_))
			{
				vasynth.PitchBend(p.value);
			}			
	        break;
        } 
        case ControlChange:
        {
            ControlChangeEvent p = m.AsControlChange();
            switch(p.control_number)
            {
				case 1: // Modulation Wheel
					vasynth.filter_cutoff_ = ((float)p.value / 127.0f) * FILTER_CUTOFF_MAX;
                    vasynth.SetFilter();			
						 	break;	
                    break;
				{

						case 2:
						{
							switch(p.value >> 5)
							{
								case 0:
									vasynth.waveform_ = WAVE_TRI;
									break;
								case 1:
									vasynth.waveform_ = WAVE_SAW;
									break;
								case 2:
									vasynth.waveform_ = WAVE_SQUARE;
									break;
								case 3:
									vasynth.waveform_ = WAVE_POLYBLEP_SAW;
									break;
							}
							vasynth.SetWaveform();
							break;
						}
						case 3:
						{
							switch(p.value >> 5)
							{
								case 0:
									vasynth.osc2_waveform_ = WAVE_TRI;
									break;
								case 1:
									vasynth.osc2_waveform_ = WAVE_SAW;
									break;
								case 2:
									vasynth.osc2_waveform_ = WAVE_SQUARE;
									break;
								case 3:
									vasynth.osc2_waveform_ = WAVE_POLYBLEP_SAW;
									break;
							}
							vasynth.SetWaveform();
							break;
						}

						case 4:
						{
							vasynth.osc_mix_ = ((float)p.value / 127.0f);
							break;
						}
						case 5:
						{
							vasynth.osc2_detune_ = ((float)p.value / 255.0f);
							break;
						}
						case 6:
						{
							vasynth.osc2_transpose_ = (1.0f + ((float)p.value / 127.0f));
							break;
						}
						case 7:
						{
							vasynth.eg_f_attack_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}						

						case 8:
						{
							vasynth.eg_f_decay_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}
						case 9:	
							{
							vasynth.eg_f_sustain_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}

						case 10:
						{
							vasynth.eg_f_release_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}
						case 11:
						{
							vasynth.eg_a_attack_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}


						case 12:
						{
							vasynth.eg_a_decay_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}
						case 13:
						{
							vasynth.eg_a_sustain_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}
						case 14:
						{
							vasynth.eg_a_release_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}
					
						case 15:
						{
							vasynth.vcf_kbd_follow_= ((float)p.value / 127.0f);
							break;
						}
						case 16:
						{
							vasynth.env_kbd_follow_ = ((float)p.value / 127.0f);
							break;
						}
					case 17:
						{
							vasynth.filter_res_ = ((float)p.value / 127.0f);
                    		vasynth.SetFilter();
							break;
						}
						case 18:
						{			
							vasynth.filter_cutoff_ = ((float)p.value / 127.0f) * FILTER_CUTOFF_MAX;
               		  	   vasynth.SetFilter();			
					 	break;
						}

						case 19:
						{
							vasynth.vcavcflfo_freq_ = ((float)p.value / 127.0f);
							vasynth.SetVCAVCFLFO();
							break;
						}
						case 20:
						{
							vasynth.vcavcflfo_amp_ = ((float)p.value / 127.0f);
							vasynth.SetVCAVCFLFO();
							break;
						}
						case 21:
						{
							switch(p.value >> 5)
							{
								case 0:
									vasynth.vcavcflfo_waveform_ = WAVE_TRI;
									break;
								case 1:
									vasynth.vcavcflfo_waveform_ = WAVE_SAW;
									break;
								case 2:
									vasynth.vcavcflfo_waveform_ = WAVE_SQUARE;
									break;
								case 3:
									vasynth.vcavcflfo_waveform_ = WAVE_POLYBLEP_SAW;
									break;
							}
								vasynth.SetVCAVCFLFO();
								break;
						}
						case 22:
						{
							// Sequencer Mode Record
							seqmode = 1;
							seqclock = 0;
							break;
						}
						case 23:
						{
							// Sequencer Mode Record End
							seqmode = 2;
							// Place a time-stamp and stop marker at the current position and stop the sequencer
							writeSramWord(seqmem, seqclock);
							seqmem++;
							seqmem++;
							writeSramWord(seqmem, 0xFFFF);
							seqmode = 0;
							seqclock = 0;
							seqmem = 0x00010000;
							break;
						}
						case 24:
						{
							// Sequencer Mode Stop
							seqmode = 0;
							seqclock = 0;
							seqmem = 0x00010000;
							break;
						}
						case 25:
						{
							// Sequencer Mode Play
							seqmode = 3;
							seqclock = 0;
							seqmem = 0x00010000;
							break;
						}
				
					}
				}	


		}
		default: break;
	}
}

void SequencerPlay(uint16_t modenum)
{
	if(modenum == 0)
	{
		seqclock = 0;
		seqmem = 0x00010000;
		return;
	}
	
	if(modenum == 3)
	{
		/* Read the time-stamp from SRAM and compare to seqclock*/
		seqtime = readSramWord(seqmem);
		
		while(seqtime == seqclock)
		{
			/* Move pointer to note data */
			seqmem++;
			seqmem++;
			
			/* Read the note from SRAM */
			seqnote = readSram(seqmem);
			seqmsg = seqnote & 0x80;
			seqnote = seqnote & 0x7F;
			seqmem++;
		
			/* Read the velocity from SRAM */
			seqvelocity = readSram(seqmem);
			seqmem++;
			
			if(seqvelocity == 0xFF)
			{
				/* End of sequence reached, reset the position to the beginning and restart */
				seqclock = 0;
				seqmem = 0x00010000;
				return;
			}
			
			if(seqmsg)
			{
				/* Play the note */
				vasynth.NoteOn(seqnote, seqvelocity);
			}
			else
			{
				/* Stop the note */
				vasynth.NoteOff(seqnote);
			}
			
			/* Check to see if there are other events at the current sequencer clock */
			seqtime = readSramWord(seqmem);
		}
	}
	
	/* All events processed so increment the sequencer clock */
	seqclock++;
}

void SequencerRecord(uint8_t recnote, uint8_t recvelocity)
{
	/* Write the timestamp to SRAM */
	writeSramWord(seqmem, seqclock);
	seqmem++;
	seqmem++;
		
	/* Write the note to SRAM */
	writeSram(seqmem, recnote);
	seqmem++;
		
	/* Write the velocity to SRAM */
	writeSram(seqmem, recvelocity);
	seqmem++;
	
	if(seqmem > 0x7FFF8)
	{
		/* There is no more memory left for the sequencer to use */
		/* Place a timestamp and stop marker at the current position and select stop mode */
		writeSramWord(seqmem, seqclock);
		seqmem++;
		seqmem++;
		writeSramWord(seqmem, 0xFFFF);
		seqmode = 0;
		seqclock = 0;
		seqmem = 0x00010000;
	}
	}

/* SRAM memory handling routines */
void writeSram(uint32_t l_addr, uint8_t l_data)
{
  /* Pointer write on specific location of backup SRAM */
  *(volatile uint8_t *) (WRITE_READ_SRAM_ADDR + l_addr) = l_data;
}

uint8_t readSram(uint32_t l_addr)
{
  uint8_t i_retval;
	
  /* Pointer write from specific location of backup SRAM */
  i_retval =  *(volatile uint8_t *) (WRITE_READ_SRAM_ADDR + l_addr);
	
  return i_retval;
}

void writeSramWord(uint32_t l_addr_w, uint16_t l_data_w)
{
  /* Pointer write on specific location of backup SRAM */
  *(volatile uint16_t *) (WRITE_READ_SRAM_ADDR + l_addr_w) = l_data_w;
}

uint16_t readSramWord(uint32_t l_addr_w)
{
  uint16_t i_retval_w;
	
  /* Pointer write from specific location of backup SRAM */
  i_retval_w =  *(volatile uint16_t *) (WRITE_READ_SRAM_ADDR + l_addr_w);
	
  return i_retval_w;
}

/* Process sequencer requests */
void Callback(void* data)
{
	SequencerPlay(seqmode);
}

void SeqTimer_Config(void)
{
	/* Create Timer Handle and Config */
    TimerHandle         tim5;
    TimerHandle::Config tim_cfg;

    /** TIM5 with IRQ enabled */
    tim_cfg.periph     = TimerHandle::Config::Peripheral::TIM_5;
    tim_cfg.enable_irq = true;

    /** Configure frequency (240Hz) */
    auto tim_target_freq = 240;
    auto tim_base_freq   = System::GetPClk2Freq();
    tim_cfg.period       = tim_base_freq / tim_target_freq;

    /** Initialize timer */
    tim5.Init(tim_cfg);
    tim5.SetCallback(Callback);

    /** Start the timer, and generate callbacks at the end of each period */
    tim5.Start();
}

int main(void)
{
	// init hardware
	hardware.Init(true); // true = boost to 480MHz
	hardware.SetAudioBlockSize(1);

	sysSampleRate = hardware.AudioSampleRate();
	sysCallbackRate = hardware.AudioCallbackRate();

	// init qspi flash for saving and loading patches
	QSPIHandle::Config qspi_config;
	qspi_config.device = QSPIHandle::Config::Device::IS25LP064A;
	qspi_config.mode   = QSPIHandle::Config::Mode::MEMORY_MAPPED;
	qspi_config.pin_config.io0 = {DSY_GPIOF, 8};
	qspi_config.pin_config.io1 = {DSY_GPIOF, 9};
	qspi_config.pin_config.io2 = {DSY_GPIOF, 7};
	qspi_config.pin_config.io3 = {DSY_GPIOF, 6};
	qspi_config.pin_config.clk = {DSY_GPIOF, 10};
	qspi_config.pin_config.ncs = {DSY_GPIOG, 6};
	hardware.qspi.Init(qspi_config);

	// setup vasynth initial values
	vasynth.Init();

	// load the default patch
	vasynth.First();

	// Initialize USB Midi 
    MidiUsbHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
	midi.Init(midi_cfg);

	// let everything settle
	System::Delay(100);
	
	// Stereo simulator
	delay_.Init();
	delay_.SetDelay(sysSampleRate * 0.01f);

	// Start the sequencer clock
	SeqTimer_Config();

	// Start calling the audio callback
	hardware.StartAudio(AudioCallback);

	// Loop forever
	for(;;)
	{
        // handle MIDI Events
        midi.Listen();
        while(midi.HasEvents())
        {
            HandleMidiMessage(midi.PopEvent());
        }	
	}}
	