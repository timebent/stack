/*******************************************************************************************************************
Copyright (c) 2023 Cycling '74

The code that Max generates automatically and that end users are capable of
exporting and using, and any associated documentation files (the “Software”)
is a work of authorship for which Cycling '74 is the author and owner for
copyright purposes.

This Software is dual-licensed either under the terms of the Cycling '74
License for Max-Generated Code for Export, or alternatively under the terms
of the General Public License (GPL) Version 3. You may use the Software
according to either of these licenses as it is most appropriate for your
project on a case-by-case basis (proprietary or not).

A) Cycling '74 License for Max-Generated Code for Export

A license is hereby granted, free of charge, to any person obtaining a copy
of the Software (“Licensee”) to use, copy, modify, merge, publish, and
distribute copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The Software is licensed to Licensee for all uses that do not include the sale,
sublicensing, or commercial distribution of software that incorporates this
source code. This means that the Licensee is free to use this software for
educational, research, and prototyping purposes, to create musical or other
creative works with software that incorporates this source code, or any other
use that does not constitute selling software that makes use of this source
code. Commercial distribution also includes the packaging of free software with
other paid software, hardware, or software-provided commercial services.

For entities with UNDER $200k in annual revenue or funding, a license is hereby
granted, free of charge, for the sale, sublicensing, or commercial distribution
of software that incorporates this source code, for as long as the entity's
annual revenue remains below $200k annual revenue or funding.

For entities with OVER $200k in annual revenue or funding interested in the
sale, sublicensing, or commercial distribution of software that incorporates
this source code, please send inquiries to licensing@cycling74.com.

The above copyright notice and this license shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Please see
https://support.cycling74.com/hc/en-us/articles/10730637742483-RNBO-Export-Licensing-FAQ
for additional information

B) General Public License Version 3 (GPLv3)
Details of the GPLv3 license can be found at: https://www.gnu.org/licenses/gpl-3.0.html
*******************************************************************************************************************/

#ifdef RNBO_LIB_PREFIX
#define STR_IMPL(A) #A
#define STR(A) STR_IMPL(A)
#define RNBO_LIB_INCLUDE(X) STR(RNBO_LIB_PREFIX/X)
#else
#define RNBO_LIB_INCLUDE(X) #X
#endif // RNBO_LIB_PREFIX
#ifdef RNBO_INJECTPLATFORM
#define RNBO_USECUSTOMPLATFORM
#include RNBO_INJECTPLATFORM
#endif // RNBO_INJECTPLATFORM

#include RNBO_LIB_INCLUDE(RNBO_Common.h)
#include RNBO_LIB_INCLUDE(RNBO_AudioSignal.h)

namespace RNBO {


#define trunc(x) ((Int)(x))
#define autoref auto&

#if defined(__GNUC__) || defined(__clang__)
    #define RNBO_RESTRICT __restrict__
#elif defined(_MSC_VER)
    #define RNBO_RESTRICT __restrict
#endif

#define FIXEDSIZEARRAYINIT(...) { }

template <class ENGINE = INTERNALENGINE> class rnbomatic : public PatcherInterfaceImpl {

friend class EngineCore;
friend class Engine;
friend class MinimalEngine<>;
public:

rnbomatic()
: _internalEngine(this)
{
}

~rnbomatic()
{
    deallocateSignals();
}

Index getNumMidiInputPorts() const {
    return 1;
}

void processMidiEvent(MillisecondTime time, int port, ConstByteArray data, Index length) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->midiin_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
}

Index getNumMidiOutputPorts() const {
    return 1;
}

void process(
    const SampleValue * const* inputs,
    Index numInputs,
    SampleValue * const* outputs,
    Index numOutputs,
    Index n
) {
    RNBO_UNUSED(numInputs);
    RNBO_UNUSED(inputs);
    this->vs = n;
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr, true);
    SampleValue * out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);
    SampleValue * out2 = (numOutputs >= 2 && outputs[1] ? outputs[1] : this->dummyBuffer);
    this->poly_perform(out1, out2, n);
    this->stackprotect_perform(n);
    this->globaltransport_advance();
    this->advanceTime((ENGINE*)nullptr);
    this->audioProcessSampleCount += this->vs;
}

void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
    RNBO_ASSERT(this->_isInitialized);

    if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
        this->globaltransport_tempo = resizeSignal(this->globaltransport_tempo, this->maxvs, maxBlockSize);
        this->globaltransport_state = resizeSignal(this->globaltransport_state, this->maxvs, maxBlockSize);
        this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
        this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
        this->didAllocateSignals = true;
    }

    const bool sampleRateChanged = sampleRate != this->sr;
    const bool maxvsChanged = maxBlockSize != this->maxvs;
    const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;

    if (sampleRateChanged || maxvsChanged) {
        this->vs = maxBlockSize;
        this->maxvs = maxBlockSize;
        this->sr = sampleRate;
        this->invsr = 1 / sampleRate;
    }

    this->globaltransport_dspsetup(forceDSPSetup);

    for (Index i = 0; i < 16; i++) {
        this->poly[i]->prepareToProcess(sampleRate, maxBlockSize, force);
    }

    if (sampleRateChanged)
        this->onSampleRateChanged(sampleRate);
}

number msToSamps(MillisecondTime ms, number sampleRate) {
    return ms * sampleRate * 0.001;
}

MillisecondTime sampsToMs(SampleIndex samps) {
    return samps * (this->invsr * 1000);
}

Index getNumInputChannels() const {
    return 0;
}

Index getNumOutputChannels() const {
    return 2;
}

DataRef* getDataRef(DataRefIndex index)  {
    switch (index) {
    case 0:
        {
        return addressOf(this->RNBODefaultMtofLookupTable256);
        break;
        }
    case 1:
        {
        return addressOf(this->RNBODefaultSinus);
        break;
        }
    default:
        {
        return nullptr;
        }
    }
}

DataRefIndex getNumDataRefs() const {
    return 2;
}

void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
    for (Index i = 0; i < 16; i++) {
        this->poly[i]->processDataViewUpdate(index, time);
    }
}

void initialize() {
    RNBO_ASSERT(!this->_isInitialized);

    this->RNBODefaultMtofLookupTable256 = initDataRef(
        this->RNBODefaultMtofLookupTable256,
        this->dataRefStrings->name0,
        true,
        this->dataRefStrings->file0,
        this->dataRefStrings->tag0
    );

    this->RNBODefaultSinus = initDataRef(
        this->RNBODefaultSinus,
        this->dataRefStrings->name1,
        true,
        this->dataRefStrings->file1,
        this->dataRefStrings->tag1
    );

    this->assign_defaults();
    this->applyState();
    this->RNBODefaultMtofLookupTable256->setIndex(0);
    this->RNBODefaultSinus->setIndex(1);
    this->initializeObjects();
    this->allocateDataRefs();
    this->startup();
    this->_isInitialized = true;
}

void getPreset(PatcherStateInterface& preset) {
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);
    preset["__presetid"] = "rnbo";
    this->param_03_getPresetValue(getSubState(preset, "index"));
    this->param_04_getPresetValue(getSubState(preset, "volume"));

    for (Index i = 0; i < 16; i++)
        this->poly[i]->getPreset(getSubStateAt(getSubState(preset, "__sps"), "poly", i));
}

void setPreset(MillisecondTime time, PatcherStateInterface& preset) {
    this->updateTime(time, (ENGINE*)nullptr);

    for (Index i = 0; i < 16; i++)
        this->poly[i]->setPreset(time, getSubStateAt(getSubState(preset, "__sps"), "poly", i));

    this->param_03_setPresetValue(getSubState(preset, "index"));
    this->param_04_setPresetValue(getSubState(preset, "volume"));
}

void setParameterValue(ParameterIndex index, ParameterValue v, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (index) {
    case 0:
        {
        this->param_03_value_set(v);
        break;
        }
    case 1:
        {
        this->param_04_value_set(v);
        break;
        }
    default:
        {
        index -= 2;

        if (index < this->poly[0]->getNumParameters())
            this->poly[0]->setPolyParameterValue(this->poly, index, v, time);

        break;
        }
    }
}

void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
    this->setParameterValue(index, value, time);
}

void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
    this->setParameterValue(index, this->getParameterValue(index), time);
}

void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
    this->setParameterValueNormalized(index, value, time);
}

ParameterValue getParameterValue(ParameterIndex index)  {
    switch (index) {
    case 0:
        {
        return this->param_03_value;
        }
    case 1:
        {
        return this->param_04_value;
        }
    default:
        {
        index -= 2;

        if (index < this->poly[0]->getNumParameters())
            return this->poly[0]->getPolyParameterValue(this->poly, index);

        return 0;
        }
    }
}

ParameterIndex getNumSignalInParameters() const {
    return 0;
}

ParameterIndex getNumSignalOutParameters() const {
    return 0;
}

ParameterIndex getNumParameters() const {
    return 2 + this->poly[0]->getNumParameters();
}

ConstCharPointer getParameterName(ParameterIndex index) const {
    switch (index) {
    case 0:
        {
        return "index";
        }
    case 1:
        {
        return "volume";
        }
    default:
        {
        index -= 2;

        if (index < this->poly[0]->getNumParameters()) {
            {
                return this->poly[0]->getParameterName(index);
            }
        }

        return "bogus";
        }
    }
}

ConstCharPointer getParameterId(ParameterIndex index) const {
    switch (index) {
    case 0:
        {
        return "index";
        }
    case 1:
        {
        return "volume";
        }
    default:
        {
        index -= 2;

        if (index < this->poly[0]->getNumParameters()) {
            {
                return this->poly[0]->getParameterId(index);
            }
        }

        return "bogus";
        }
    }
}

void getParameterInfo(ParameterIndex index, ParameterInfo * info) const {
    {
        switch (index) {
        case 0:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 1;
            info->min = 1;
            info->max = 7;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "";
            info->unit = "";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 1:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 0.1;
            info->min = 0;
            info->max = 1;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "";
            info->unit = "";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        default:
            {
            index -= 2;

            if (index < this->poly[0]->getNumParameters()) {
                for (Index i = 0; i < 16; i++) {
                    this->poly[i]->getParameterInfo(index, info);
                }
            }

            break;
            }
        }
    }
}

ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
    if (steps == 1) {
        if (normalizedValue > 0) {
            normalizedValue = 1.;
        }
    } else {
        ParameterValue oneStep = (number)1. / (steps - 1);
        ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
        normalizedValue = numberOfSteps * oneStep;
    }

    return normalizedValue;
}

ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
    switch (index) {
    case 1:
        {
        {
            value = (value < 0 ? 0 : (value > 1 ? 1 : value));
            ParameterValue normalizedValue = (value - 0) / (1 - 0);
            return normalizedValue;
        }
        }
    case 0:
        {
        {
            value = (value < 1 ? 1 : (value > 7 ? 7 : value));
            ParameterValue normalizedValue = (value - 1) / (7 - 1);
            return normalizedValue;
        }
        }
    default:
        {
        index -= 2;

        if (index < this->poly[0]->getNumParameters()) {
            {
                return this->poly[0]->convertToNormalizedParameterValue(index, value);
            }
        }

        return value;
        }
    }
}

ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

    switch (index) {
    case 1:
        {
        {
            {
                return 0 + value * (1 - 0);
            }
        }
        }
    case 0:
        {
        {
            {
                return 1 + value * (7 - 1);
            }
        }
        }
    default:
        {
        index -= 2;

        if (index < this->poly[0]->getNumParameters()) {
            {
                return this->poly[0]->convertFromNormalizedParameterValue(index, value);
            }
        }

        return value;
        }
    }
}

ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
    switch (index) {
    case 0:
        {
        return this->param_03_value_constrain(value);
        }
    case 1:
        {
        return this->param_04_value_constrain(value);
        }
    default:
        {
        index -= 2;

        if (index < this->poly[0]->getNumParameters()) {
            {
                return this->poly[0]->constrainParameterValue(index, value);
            }
        }

        return value;
        }
    }
}

void processNumMessage(MessageTag tag, MessageTag objectId, MillisecondTime time, number payload) {
    RNBO_UNUSED(objectId);
    this->updateTime(time, (ENGINE*)nullptr);

    for (Index i = 0; i < 16; i++) {
        this->poly[i]->processNumMessage(tag, objectId, time, payload);
    }
}

void processListMessage(
    MessageTag tag,
    MessageTag objectId,
    MillisecondTime time,
    const list& payload
) {
    RNBO_UNUSED(objectId);
    this->updateTime(time, (ENGINE*)nullptr);

    for (Index i = 0; i < 16; i++) {
        this->poly[i]->processListMessage(tag, objectId, time, payload);
    }
}

void processBangMessage(MessageTag tag, MessageTag objectId, MillisecondTime time) {
    RNBO_UNUSED(objectId);
    this->updateTime(time, (ENGINE*)nullptr);

    for (Index i = 0; i < 16; i++) {
        this->poly[i]->processBangMessage(tag, objectId, time);
    }
}

MessageTagInfo resolveTag(MessageTag tag) const {
    switch (tag) {
    case TAG("out3"):
        {
        return "out3";
        }
    case TAG(""):
        {
        return "";
        }
    }

    auto subpatchResult_0 = this->poly[0]->resolveTag(tag);

    if (subpatchResult_0)
        return subpatchResult_0;

    return "";
}

MessageIndex getNumMessages() const {
    return 1;
}

const MessageInfo& getMessageInfo(MessageIndex index) const {
    switch (index) {
    case 0:
        {
        static const MessageInfo r0 = {
            "out3",
            Outport
        };

        return r0;
        }
    }

    return NullMessageInfo;
}

protected:

class RNBOSubpatcher_03 : public PatcherInterfaceImpl {
    
    friend class rnbomatic;
    
    public:
    
    RNBOSubpatcher_03()
    {}
    
    ~RNBOSubpatcher_03()
    {
        deallocateSignals();
    }
    
    Index getNumMidiInputPorts() const {
        return 1;
    }
    
    void processMidiEvent(MillisecondTime time, int port, ConstByteArray data, Index length) {
        this->updateTime(time, (ENGINE*)nullptr);
        this->notein_01_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
    }
    
    Index getNumMidiOutputPorts() const {
        return 0;
    }
    
    void process(
        const SampleValue * const* inputs,
        Index numInputs,
        SampleValue * const* outputs,
        Index numOutputs,
        Index n
    ) {
        RNBO_UNUSED(numInputs);
        RNBO_UNUSED(inputs);
        this->vs = n;
        this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr, true);
        SampleValue * out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);
        SampleValue * out2 = (numOutputs >= 2 && outputs[1] ? outputs[1] : this->dummyBuffer);
    
        if (this->getIsMuted())
            return;
    
        this->cycle_tilde_02_perform(
            this->cycle_tilde_02_frequency,
            this->cycle_tilde_02_phase_offset,
            this->signals[0],
            this->dummyBuffer,
            n
        );
    
        this->dspexpr_04_perform(this->dspexpr_04_in1, this->dspexpr_04_in2, this->signals[1], n);
        this->dspexpr_02_perform(this->signals[0], this->signals[1], this->signals[2], n);
    
        this->cycle_tilde_01_perform(
            this->signals[2],
            this->cycle_tilde_01_phase_offset,
            this->signals[1],
            this->dummyBuffer,
            n
        );
    
        this->adsr_01_perform(
            this->adsr_01_attack,
            this->adsr_01_decay,
            this->adsr_01_sustain,
            this->adsr_01_release,
            this->zeroBuffer,
            this->signals[2],
            n
        );
    
        this->dspexpr_01_perform(this->signals[1], this->signals[2], this->signals[0], n);
        this->dspexpr_03_perform(this->signals[0], this->dspexpr_03_in2, this->signals[2], n);
        this->signaladder_01_perform(this->signals[2], out1, out1, n);
        this->signaladder_02_perform(this->signals[2], out2, out2, n);
        this->stackprotect_perform(n);
        this->audioProcessSampleCount += this->vs;
    }
    
    void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
        RNBO_ASSERT(this->_isInitialized);
    
        if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
            Index i;
    
            for (i = 0; i < 3; i++) {
                this->signals[i] = resizeSignal(this->signals[i], this->maxvs, maxBlockSize);
            }
    
            this->adsr_01_triggerBuf = resizeSignal(this->adsr_01_triggerBuf, this->maxvs, maxBlockSize);
            this->adsr_01_triggerValueBuf = resizeSignal(this->adsr_01_triggerValueBuf, this->maxvs, maxBlockSize);
            this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
            this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
            this->didAllocateSignals = true;
        }
    
        const bool sampleRateChanged = sampleRate != this->sr;
        const bool maxvsChanged = maxBlockSize != this->maxvs;
        const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;
    
        if (sampleRateChanged || maxvsChanged) {
            this->vs = maxBlockSize;
            this->maxvs = maxBlockSize;
            this->sr = sampleRate;
            this->invsr = 1 / sampleRate;
        }
    
        this->cycle_tilde_02_dspsetup(forceDSPSetup);
        this->cycle_tilde_01_dspsetup(forceDSPSetup);
        this->adsr_01_dspsetup(forceDSPSetup);
    
        if (sampleRateChanged)
            this->onSampleRateChanged(sampleRate);
    }
    
    number msToSamps(MillisecondTime ms, number sampleRate) {
        return ms * sampleRate * 0.001;
    }
    
    MillisecondTime sampsToMs(SampleIndex samps) {
        return samps * (this->invsr * 1000);
    }
    
    Index getNumInputChannels() const {
        return 0;
    }
    
    Index getNumOutputChannels() const {
        return 2;
    }
    
    void getPreset(PatcherStateInterface& ) {}
    
    void setPreset(MillisecondTime , PatcherStateInterface& ) {}
    
    void setParameterValue(ParameterIndex index, ParameterValue v, MillisecondTime time) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (index) {
        case 0:
            {
            this->param_01_value_set(v);
            break;
            }
        case 1:
            {
            this->param_02_value_set(v);
            break;
            }
        }
    }
    
    void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValue(index, value, time);
    }
    
    void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
        this->setParameterValue(index, this->getParameterValue(index), time);
    }
    
    void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
        this->setParameterValueNormalized(index, value, time);
    }
    
    ParameterValue getParameterValue(ParameterIndex index)  {
        switch (index) {
        case 0:
            {
            return this->param_01_value;
            }
        case 1:
            {
            return this->param_02_value;
            }
        default:
            {
            return 0;
            }
        }
    }
    
    ParameterIndex getNumSignalInParameters() const {
        return 0;
    }
    
    ParameterIndex getNumSignalOutParameters() const {
        return 0;
    }
    
    ParameterIndex getNumParameters() const {
        return 2;
    }
    
    ConstCharPointer getParameterName(ParameterIndex index) const {
        switch (index) {
        case 0:
            {
            return "index";
            }
        case 1:
            {
            return "volume";
            }
        default:
            {
            return "bogus";
            }
        }
    }
    
    ConstCharPointer getParameterId(ParameterIndex index) const {
        switch (index) {
        case 0:
            {
            return "poly/index";
            }
        case 1:
            {
            return "poly/volume";
            }
        default:
            {
            return "bogus";
            }
        }
    }
    
    void getParameterInfo(ParameterIndex index, ParameterInfo * info) const {
        {
            switch (index) {
            case 0:
                {
                info->type = ParameterTypeNumber;
                info->initialValue = 1;
                info->min = 1;
                info->max = 7;
                info->exponent = 1;
                info->steps = 0;
                info->debug = false;
                info->saveable = true;
                info->transmittable = true;
                info->initialized = true;
                info->visible = false;
                info->displayName = "";
                info->unit = "";
                info->ioType = IOTypeUndefined;
                info->signalIndex = INVALID_INDEX;
                break;
                }
            case 1:
                {
                info->type = ParameterTypeNumber;
                info->initialValue = 0.1;
                info->min = 0;
                info->max = 1;
                info->exponent = 1;
                info->steps = 0;
                info->debug = false;
                info->saveable = true;
                info->transmittable = true;
                info->initialized = true;
                info->visible = false;
                info->displayName = "";
                info->unit = "";
                info->ioType = IOTypeUndefined;
                info->signalIndex = INVALID_INDEX;
                break;
                }
            }
        }
    }
    
    ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
        if (steps == 1) {
            if (normalizedValue > 0) {
                normalizedValue = 1.;
            }
        } else {
            ParameterValue oneStep = (number)1. / (steps - 1);
            ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
            normalizedValue = numberOfSteps * oneStep;
        }
    
        return normalizedValue;
    }
    
    ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        case 1:
            {
            {
                value = (value < 0 ? 0 : (value > 1 ? 1 : value));
                ParameterValue normalizedValue = (value - 0) / (1 - 0);
                return normalizedValue;
            }
            }
        case 0:
            {
            {
                value = (value < 1 ? 1 : (value > 7 ? 7 : value));
                ParameterValue normalizedValue = (value - 1) / (7 - 1);
                return normalizedValue;
            }
            }
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
        value = (value < 0 ? 0 : (value > 1 ? 1 : value));
    
        switch (index) {
        case 1:
            {
            {
                {
                    return 0 + value * (1 - 0);
                }
            }
            }
        case 0:
            {
            {
                {
                    return 1 + value * (7 - 1);
                }
            }
            }
        default:
            {
            return value;
            }
        }
    }
    
    ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
        switch (index) {
        case 0:
            {
            return this->param_01_value_constrain(value);
            }
        case 1:
            {
            return this->param_02_value_constrain(value);
            }
        default:
            {
            return value;
            }
        }
    }
    
    void processNumMessage(MessageTag , MessageTag , MillisecondTime , number ) {}
    
    void processListMessage(MessageTag , MessageTag , MillisecondTime , const list& ) {}
    
    void processBangMessage(MessageTag , MessageTag , MillisecondTime ) {}
    
    MessageTagInfo resolveTag(MessageTag tag) const {
        switch (tag) {
    
        }
    
        return nullptr;
    }
    
    DataRef* getDataRef(DataRefIndex index)  {
        switch (index) {
        default:
            {
            return nullptr;
            }
        }
    }
    
    DataRefIndex getNumDataRefs() const {
        return 0;
    }
    
    void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
        this->updateTime(time, (ENGINE*)nullptr);
    
        if (index == 0) {
            this->mtof_01_innerMtoF_buffer = reInitDataView(
                this->mtof_01_innerMtoF_buffer,
                this->getPatcher()->RNBODefaultMtofLookupTable256
            );
        }
    
        if (index == 1) {
            this->cycle_tilde_01_buffer = reInitDataView(this->cycle_tilde_01_buffer, this->getPatcher()->RNBODefaultSinus);
            this->cycle_tilde_01_bufferUpdated();
            this->cycle_tilde_02_buffer = reInitDataView(this->cycle_tilde_02_buffer, this->getPatcher()->RNBODefaultSinus);
            this->cycle_tilde_02_bufferUpdated();
        }
    }
    
    void initialize() {
        RNBO_ASSERT(!this->_isInitialized);
        this->assign_defaults();
        this->applyState();
        this->mtof_01_innerMtoF_buffer = new SampleBuffer(this->getPatcher()->RNBODefaultMtofLookupTable256);
        this->cycle_tilde_01_buffer = new SampleBuffer(this->getPatcher()->RNBODefaultSinus);
        this->cycle_tilde_02_buffer = new SampleBuffer(this->getPatcher()->RNBODefaultSinus);
        this->_isInitialized = true;
    }
    
    protected:
    
    void updateTime(MillisecondTime time, INTERNALENGINE*, bool inProcess = false) {
    	if (time == TimeNow) time = getTopLevelPatcher()->getPatcherTime();
    	getTopLevelPatcher()->processInternalEvents(time);
    	updateTime(time, (EXTERNALENGINE*)nullptr);
    }
    
    RNBOSubpatcher_03* operator->() {
        return this;
    }
    const RNBOSubpatcher_03* operator->() const {
        return this;
    }
    virtual rnbomatic* getPatcher() const {
        return static_cast<rnbomatic *>(_parentPatcher);
    }
    
    rnbomatic* getTopLevelPatcher() {
        return this->getPatcher()->getTopLevelPatcher();
    }
    
    void cancelClockEvents()
    {
        getEngine()->flushClockEvents(this, -1468824490, false);
    }
    
    inline number linearinterp(number frac, number x, number y) {
        return x + (y - x) * frac;
    }
    
    MillisecondTime getPatcherTime() const {
        return this->_currentTime;
    }
    
    void param_01_value_set(number v) {
        v = this->param_01_value_constrain(v);
        this->param_01_value = v;
        this->sendParameter(0, false);
    
        if (this->param_01_value != this->param_01_lastValue) {
            this->getEngine()->presetTouched();
            this->param_01_lastValue = this->param_01_value;
        }
    
        this->dspexpr_04_in2_set(v);
    }
    
    void param_02_value_set(number v) {
        v = this->param_02_value_constrain(v);
        this->param_02_value = v;
        this->sendParameter(1, false);
    
        if (this->param_02_value != this->param_02_lastValue) {
            this->getEngine()->presetTouched();
            this->param_02_lastValue = this->param_02_value;
        }
    
        this->dspexpr_03_in2_set(v);
    }
    
    void adsr_01_mute_bang() {}
    
    void deallocateSignals() {
        Index i;
    
        for (i = 0; i < 3; i++) {
            this->signals[i] = freeSignal(this->signals[i]);
        }
    
        this->adsr_01_triggerBuf = freeSignal(this->adsr_01_triggerBuf);
        this->adsr_01_triggerValueBuf = freeSignal(this->adsr_01_triggerValueBuf);
        this->zeroBuffer = freeSignal(this->zeroBuffer);
        this->dummyBuffer = freeSignal(this->dummyBuffer);
    }
    
    Index getMaxBlockSize() const {
        return this->maxvs;
    }
    
    number getSampleRate() const {
        return this->sr;
    }
    
    bool hasFixedVectorSize() const {
        return false;
    }
    
    void setProbingTarget(MessageTag ) {}
    
    void initializeObjects() {
        this->mtof_01_innerScala_init();
        this->mtof_01_init();
    }
    
    void setVoiceIndex(Index index)  {
        this->_voiceIndex = index;
    }
    
    void setNoteNumber(Int noteNumber)  {
        this->_noteNumber = noteNumber;
    }
    
    Index getIsMuted()  {
        return this->isMuted;
    }
    
    void setIsMuted(Index v)  {
        this->isMuted = v;
    }
    
    void onSampleRateChanged(double ) {}
    
    void extractState(PatcherStateInterface& ) {}
    
    void applyState() {}
    
    ParameterValue getPolyParameterValue(RNBOSubpatcher_03* voices, ParameterIndex index)  {
        switch (index) {
        default:
            {
            return voices[0]->getParameterValue(index);
            }
        }
    }
    
    void setPolyParameterValue(
        RNBOSubpatcher_03* voices,
        ParameterIndex index,
        ParameterValue value,
        MillisecondTime time
    ) {
        switch (index) {
        default:
            {
            for (Index i = 0; i < 16; i++)
                voices[i]->setParameterValue(index, value, time);
            }
        }
    }
    
    void sendPolyParameter(ParameterIndex index, Index voiceIndex, bool ignoreValue) {
        this->getPatcher()->sendParameter(index + this->parameterOffset + voiceIndex - 1, ignoreValue);
    }
    
    void setParameterOffset(ParameterIndex offset) {
        this->parameterOffset = offset;
    }
    
    void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {
        RNBO_UNUSED(value);
        RNBO_UNUSED(hasValue);
        this->updateTime(time, (ENGINE*)nullptr);
    
        switch (index) {
        case -1468824490:
            {
            this->adsr_01_mute_bang();
            break;
            }
        }
    }
    
    void processOutletAtCurrentTime(EngineLink* , OutletIndex , ParameterValue ) {}
    
    void processOutletEvent(
        EngineLink* sender,
        OutletIndex index,
        ParameterValue value,
        MillisecondTime time
    ) {
        this->updateTime(time, (ENGINE*)nullptr);
        this->processOutletAtCurrentTime(sender, index, value);
    }
    
    void sendOutlet(OutletIndex index, ParameterValue value) {
        this->getEngine()->sendOutlet(this, index, value);
    }
    
    void startup() {}
    
    void fillDataRef(DataRefIndex , DataRef& ) {}
    
    void allocateDataRefs() {
        this->mtof_01_innerMtoF_buffer->requestSize(65536, 1);
        this->mtof_01_innerMtoF_buffer->setSampleRate(this->sr);
        this->cycle_tilde_01_buffer->requestSize(16384, 1);
        this->cycle_tilde_01_buffer->setSampleRate(this->sr);
        this->cycle_tilde_02_buffer->requestSize(16384, 1);
        this->cycle_tilde_02_buffer->setSampleRate(this->sr);
        this->mtof_01_innerMtoF_buffer = this->mtof_01_innerMtoF_buffer->allocateIfNeeded();
        this->cycle_tilde_01_buffer = this->cycle_tilde_01_buffer->allocateIfNeeded();
        this->cycle_tilde_02_buffer = this->cycle_tilde_02_buffer->allocateIfNeeded();
    }
    
    number param_01_value_constrain(number v) const {
        v = (v > 7 ? 7 : (v < 1 ? 1 : v));
        return v;
    }
    
    void dspexpr_04_in2_set(number v) {
        this->dspexpr_04_in2 = v;
    }
    
    number param_02_value_constrain(number v) const {
        v = (v > 1 ? 1 : (v < 0 ? 0 : v));
        return v;
    }
    
    void dspexpr_03_in2_set(number v) {
        this->dspexpr_03_in2 = v;
    }
    
    void notein_01_outchannel_set(number ) {}
    
    void notein_01_releasevelocity_set(number ) {}
    
    void adsr_01_trigger_number_set(number v) {
        this->adsr_01_trigger_number = v;
    
        if (v != 0)
            this->adsr_01_triggerBuf[this->sampleOffsetIntoNextAudioBuffer] = 1;
    
        for (number i = this->sampleOffsetIntoNextAudioBuffer; i < this->vs; i++) {
            this->adsr_01_triggerValueBuf[(Index)i] = v;
        }
    }
    
    void expr_01_out1_set(number v) {
        this->expr_01_out1 = v;
        this->adsr_01_trigger_number_set(this->expr_01_out1);
    }
    
    void expr_01_in1_set(number in1) {
        this->expr_01_in1 = in1;
    
        this->expr_01_out1_set(
            (this->expr_01_in2 == 0 ? 0 : (this->expr_01_in2 == 0. ? 0. : this->expr_01_in1 / this->expr_01_in2))
        );//#map:/_obj-7:1
    }
    
    void notein_01_velocity_set(number v) {
        this->expr_01_in1_set(v);
    }
    
    template<typename LISTTYPE> void eventoutlet_01_in1_list_set(const LISTTYPE& v) {
        this->getPatcher()->updateTime(this->_currentTime, (ENGINE*)nullptr);
        this->getPatcher()->poly_out3_list_set((list)v);
    }
    
    void dspexpr_04_in1_set(number v) {
        this->dspexpr_04_in1 = v;
    }
    
    void cycle_tilde_02_frequency_set(number v) {
        this->cycle_tilde_02_frequency = v;
    }
    
    void cycle_tilde_01_frequency_set(number v) {
        this->cycle_tilde_01_frequency = v;
    }
    
    void cycle_tilde_02_phase_offset_set(number v) {
        this->cycle_tilde_02_phase_offset = v;
    }
    
    void cycle_tilde_01_phase_offset_set(number v) {
        this->cycle_tilde_01_phase_offset = v;
    }
    
    template<typename LISTTYPE> void mtof_01_out_set(const LISTTYPE& v) {
        this->eventoutlet_01_in1_list_set(v);
    
        {
            if (v->length > 1)
                this->dspexpr_04_in2_set(v[1]);
    
            number converted = (v->length > 0 ? v[0] : 0);
            this->dspexpr_04_in1_set(converted);
        }
    
        {
            if (v->length > 1)
                this->cycle_tilde_02_phase_offset_set(v[1]);
    
            number converted = (v->length > 0 ? v[0] : 0);
            this->cycle_tilde_02_frequency_set(converted);
        }
    
        {
            if (v->length > 1)
                this->cycle_tilde_01_phase_offset_set(v[1]);
    
            number converted = (v->length > 0 ? v[0] : 0);
            this->cycle_tilde_01_frequency_set(converted);
        }
    }
    
    template<typename LISTTYPE> void mtof_01_midivalue_set(const LISTTYPE& v) {
        this->mtof_01_midivalue = jsCreateListCopy(v);
        list tmp = list();
    
        for (Int i = 0; i < this->mtof_01_midivalue->length; i++) {
            tmp->push(
                this->mtof_01_innerMtoF_next(this->mtof_01_midivalue[(Index)i], this->mtof_01_base)
            );
        }
    
        this->mtof_01_out_set(tmp);
    }
    
    void notein_01_notenumber_set(number v) {
        {
            listbase<number, 1> converted = {v};
            this->mtof_01_midivalue_set(converted);
        }
    }
    
    void notein_01_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
        RNBO_UNUSED(length);
        RNBO_UNUSED(port);
    
        if (channel == this->notein_01_channel || this->notein_01_channel <= 0) {
            if (status == 144 || status == 128) {
                this->notein_01_outchannel_set(channel);
    
                if (status == 128) {
                    this->notein_01_releasevelocity_set(data[2]);
                    this->notein_01_velocity_set(0);
                } else {
                    this->notein_01_releasevelocity_set(0);
                    this->notein_01_velocity_set(data[2]);
                }
    
                this->notein_01_notenumber_set(data[1]);
            }
        }
    }
    
    void midiouthelper_midiout_set(number ) {}
    
    void cycle_tilde_02_perform(
        number frequency,
        number phase_offset,
        SampleValue * out1,
        SampleValue * out2,
        Index n
    ) {
        RNBO_UNUSED(phase_offset);
        auto __cycle_tilde_02_f2i = this->cycle_tilde_02_f2i;
        auto __cycle_tilde_02_buffer = this->cycle_tilde_02_buffer;
        auto __cycle_tilde_02_phasei = this->cycle_tilde_02_phasei;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            {
                UInt32 uint_phase;
    
                {
                    {
                        uint_phase = __cycle_tilde_02_phasei;
                    }
                }
    
                UInt32 idx = (UInt32)(uint32_rshift(uint_phase, 18));
                number frac = ((BinOpInt)((BinOpInt)uint_phase & (BinOpInt)262143)) * 3.81471181759574e-6;
                number y0 = __cycle_tilde_02_buffer[(Index)idx];
                number y1 = __cycle_tilde_02_buffer[(Index)((BinOpInt)(idx + 1) & (BinOpInt)16383)];
                number y = y0 + frac * (y1 - y0);
    
                {
                    UInt32 pincr = (UInt32)(uint32_trunc(frequency * __cycle_tilde_02_f2i));
                    __cycle_tilde_02_phasei = uint32_add(__cycle_tilde_02_phasei, pincr);
                }
    
                out1[(Index)i] = y;
                out2[(Index)i] = uint_phase * 0.232830643653869629e-9;
                continue;
            }
        }
    
        this->cycle_tilde_02_phasei = __cycle_tilde_02_phasei;
    }
    
    void dspexpr_04_perform(number in1, number in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1 * in2;//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_02_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void cycle_tilde_01_perform(
        const Sample * frequency,
        number phase_offset,
        SampleValue * out1,
        SampleValue * out2,
        Index n
    ) {
        RNBO_UNUSED(phase_offset);
        auto __cycle_tilde_01_f2i = this->cycle_tilde_01_f2i;
        auto __cycle_tilde_01_buffer = this->cycle_tilde_01_buffer;
        auto __cycle_tilde_01_phasei = this->cycle_tilde_01_phasei;
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            {
                UInt32 uint_phase;
    
                {
                    {
                        uint_phase = __cycle_tilde_01_phasei;
                    }
                }
    
                UInt32 idx = (UInt32)(uint32_rshift(uint_phase, 18));
                number frac = ((BinOpInt)((BinOpInt)uint_phase & (BinOpInt)262143)) * 3.81471181759574e-6;
                number y0 = __cycle_tilde_01_buffer[(Index)idx];
                number y1 = __cycle_tilde_01_buffer[(Index)((BinOpInt)(idx + 1) & (BinOpInt)16383)];
                number y = y0 + frac * (y1 - y0);
    
                {
                    UInt32 pincr = (UInt32)(uint32_trunc(frequency[(Index)i] * __cycle_tilde_01_f2i));
                    __cycle_tilde_01_phasei = uint32_add(__cycle_tilde_01_phasei, pincr);
                }
    
                out1[(Index)i] = y;
                out2[(Index)i] = uint_phase * 0.232830643653869629e-9;
                continue;
            }
        }
    
        this->cycle_tilde_01_phasei = __cycle_tilde_01_phasei;
    }
    
    void adsr_01_perform(
        number attack,
        number decay,
        number sustain,
        number release,
        const SampleValue * trigger_signal,
        SampleValue * out,
        Index n
    ) {
        RNBO_UNUSED(trigger_signal);
        RNBO_UNUSED(release);
        RNBO_UNUSED(sustain);
        RNBO_UNUSED(decay);
        RNBO_UNUSED(attack);
        auto __adsr_01_trigger_number = this->adsr_01_trigger_number;
        auto __adsr_01_time = this->adsr_01_time;
        auto __adsr_01_amplitude = this->adsr_01_amplitude;
        auto __adsr_01_outval = this->adsr_01_outval;
        auto __adsr_01_startingpoint = this->adsr_01_startingpoint;
        auto __adsr_01_phase = this->adsr_01_phase;
        auto __adsr_01_legato = this->adsr_01_legato;
        auto __adsr_01_lastTriggerVal = this->adsr_01_lastTriggerVal;
        auto __adsr_01_maxsustain = this->adsr_01_maxsustain;
        auto __adsr_01_mspersamp = this->adsr_01_mspersamp;
        bool bangMute = false;
    
        for (Index i = 0; i < n; i++) {
            number clampedattack = (10 > __adsr_01_mspersamp ? 10 : __adsr_01_mspersamp);
            number clampeddecay = (100 > __adsr_01_mspersamp ? 100 : __adsr_01_mspersamp);
            number clampedsustain = (__adsr_01_maxsustain > __adsr_01_mspersamp ? __adsr_01_maxsustain : __adsr_01_mspersamp);
            number clampedrelease = (1000 > __adsr_01_mspersamp ? 1000 : __adsr_01_mspersamp);
            number currentTriggerVal = this->adsr_01_triggerValueBuf[(Index)i];
    
            if ((__adsr_01_lastTriggerVal == 0.0 && currentTriggerVal != 0.0) || this->adsr_01_triggerBuf[(Index)i] == 1) {
                if ((bool)(__adsr_01_legato)) {
                    if (__adsr_01_phase != 0) {
                        __adsr_01_startingpoint = __adsr_01_outval;
                    } else {
                        __adsr_01_startingpoint = 0;
                    }
                } else {
                    __adsr_01_startingpoint = 0;
                }
    
                __adsr_01_amplitude = currentTriggerVal;
                __adsr_01_phase = 3;
                __adsr_01_time = 0.0;
                bangMute = false;
            } else if (__adsr_01_lastTriggerVal != 0.0 && currentTriggerVal == 0.0) {
                if (__adsr_01_phase != 4 && __adsr_01_phase != 0) {
                    __adsr_01_phase = 4;
                    __adsr_01_amplitude = __adsr_01_outval;
                    __adsr_01_time = 0.0;
                }
            }
    
            __adsr_01_time += __adsr_01_mspersamp;
    
            if (__adsr_01_phase == 0) {
                __adsr_01_outval = 0;
            } else if (__adsr_01_phase == 3) {
                if (__adsr_01_time > clampedattack) {
                    __adsr_01_time -= clampedattack;
                    __adsr_01_phase = 2;
                    __adsr_01_outval = __adsr_01_amplitude;
                } else {
                    __adsr_01_outval = (__adsr_01_amplitude - __adsr_01_startingpoint) * __adsr_01_time / clampedattack + __adsr_01_startingpoint;
                }
            } else if (__adsr_01_phase == 2) {
                if (__adsr_01_time > clampeddecay) {
                    __adsr_01_time -= clampeddecay;
                    __adsr_01_phase = 1;
                    __adsr_01_outval = __adsr_01_amplitude * 0.8;
                } else {
                    __adsr_01_outval = __adsr_01_amplitude * 0.8 + (__adsr_01_amplitude - __adsr_01_amplitude * 0.8) * (1. - __adsr_01_time / clampeddecay);
                }
            } else if (__adsr_01_phase == 1) {
                if (__adsr_01_time > clampedsustain && __adsr_01_maxsustain > -1) {
                    __adsr_01_time -= clampedsustain;
                    __adsr_01_phase = 4;
                    __adsr_01_amplitude = __adsr_01_outval;
                } else {
                    __adsr_01_outval = __adsr_01_amplitude * 0.8;
                }
            } else if (__adsr_01_phase == 4) {
                if (__adsr_01_time > clampedrelease) {
                    __adsr_01_time = 0;
                    __adsr_01_phase = 0;
                    __adsr_01_outval = 0;
                    __adsr_01_amplitude = 0;
                    bangMute = true;
                } else {
                    __adsr_01_outval = __adsr_01_amplitude * (1.0 - __adsr_01_time / clampedrelease);
                }
            }
    
            out[(Index)i] = __adsr_01_outval;
            this->adsr_01_triggerBuf[(Index)i] = 0;
            this->adsr_01_triggerValueBuf[(Index)i] = __adsr_01_trigger_number;
            __adsr_01_lastTriggerVal = currentTriggerVal;
        }
    
        if ((bool)(bangMute)) {
            this->getEngine()->scheduleClockEventWithValue(
                this,
                -1468824490,
                this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
                0
            );;
        }
    
        this->adsr_01_lastTriggerVal = __adsr_01_lastTriggerVal;
        this->adsr_01_phase = __adsr_01_phase;
        this->adsr_01_startingpoint = __adsr_01_startingpoint;
        this->adsr_01_outval = __adsr_01_outval;
        this->adsr_01_amplitude = __adsr_01_amplitude;
        this->adsr_01_time = __adsr_01_time;
    }
    
    void dspexpr_01_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
        }
    }
    
    void dspexpr_03_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
        }
    }
    
    void signaladder_01_perform(
        const SampleValue * in1,
        const SampleValue * in2,
        SampleValue * out,
        Index n
    ) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out[(Index)i] = in1[(Index)i] + in2[(Index)i];
        }
    }
    
    void signaladder_02_perform(
        const SampleValue * in1,
        const SampleValue * in2,
        SampleValue * out,
        Index n
    ) {
        Index i;
    
        for (i = 0; i < (Index)n; i++) {
            out[(Index)i] = in1[(Index)i] + in2[(Index)i];
        }
    }
    
    void stackprotect_perform(Index n) {
        RNBO_UNUSED(n);
        auto __stackprotect_count = this->stackprotect_count;
        __stackprotect_count = 0;
        this->stackprotect_count = __stackprotect_count;
    }
    
    number mtof_01_innerMtoF_next(number midivalue, number tuning) {
        if (midivalue == this->mtof_01_innerMtoF_lastInValue && tuning == this->mtof_01_innerMtoF_lastTuning)
            return this->mtof_01_innerMtoF_lastOutValue;
    
        this->mtof_01_innerMtoF_lastInValue = midivalue;
        this->mtof_01_innerMtoF_lastTuning = tuning;
        number result = 0;
    
        {
            result = rnbo_exp(.057762265 * (midivalue - 69.0));
        }
    
        this->mtof_01_innerMtoF_lastOutValue = tuning * result;
        return this->mtof_01_innerMtoF_lastOutValue;
    }
    
    void mtof_01_innerMtoF_reset() {
        this->mtof_01_innerMtoF_lastInValue = 0;
        this->mtof_01_innerMtoF_lastOutValue = 0;
        this->mtof_01_innerMtoF_lastTuning = 0;
    }
    
    void mtof_01_innerScala_mid(Int v) {
        this->mtof_01_innerScala_kbmMid = v;
        this->mtof_01_innerScala_updateRefFreq();
    }
    
    void mtof_01_innerScala_ref(Int v) {
        this->mtof_01_innerScala_kbmRefNum = v;
        this->mtof_01_innerScala_updateRefFreq();
    }
    
    void mtof_01_innerScala_base(number v) {
        this->mtof_01_innerScala_kbmRefFreq = v;
        this->mtof_01_innerScala_updateRefFreq();
    }
    
    void mtof_01_innerScala_init() {
        list sclValid = {
            12,
            100,
            0,
            200,
            0,
            300,
            0,
            400,
            0,
            500,
            0,
            600,
            0,
            700,
            0,
            800,
            0,
            900,
            0,
            1000,
            0,
            1100,
            0,
            2,
            1
        };
    
        this->mtof_01_innerScala_updateScale(sclValid);
    }
    
    template<typename LISTTYPE = list> void mtof_01_innerScala_update(const LISTTYPE& scale, const LISTTYPE& map) {
        if (scale->length > 0) {
            this->mtof_01_innerScala_updateScale(scale);
        }
    
        if (map->length > 0) {
            this->mtof_01_innerScala_updateMap(map);
        }
    }
    
    number mtof_01_innerScala_mtof(number note) {
        if ((bool)(this->mtof_01_innerScala_lastValid) && this->mtof_01_innerScala_lastNote == note) {
            return this->mtof_01_innerScala_lastFreq;
        }
    
        array<Int, 2> degoct = this->mtof_01_innerScala_applyKBM(note);
        number out = 0;
    
        if (degoct[1] > 0) {
            out = this->mtof_01_innerScala_applySCL(degoct[0], fract(note), this->mtof_01_innerScala_refFreq);
        }
    
        this->mtof_01_innerScala_updateLast(note, out);
        return out;
    }
    
    number mtof_01_innerScala_ftom(number hz) {
        if (hz <= 0.0) {
            return 0.0;
        }
    
        if ((bool)(this->mtof_01_innerScala_lastValid) && this->mtof_01_innerScala_lastFreq == hz) {
            return this->mtof_01_innerScala_lastNote;
        }
    
        array<number, 2> df = this->mtof_01_innerScala_hztodeg(hz);
        Int degree = (Int)(df[0]);
        number frac = df[1];
        number out = 0;
    
        if (this->mtof_01_innerScala_kbmSize == 0) {
            out = this->mtof_01_innerScala_kbmMid + degree;
        } else {
            array<Int, 2> octdeg = this->mtof_01_innerScala_octdegree(degree, this->mtof_01_innerScala_kbmOctaveDegree);
            number oct = (number)(octdeg[0]);
            Int index = (Int)(octdeg[1]);
            Index entry = 0;
    
            for (Index i = 0; i < this->mtof_01_innerScala_kbmMapSize; i++) {
                if (index == this->mtof_01_innerScala_kbmValid[(Index)(i + this->mtof_01_innerScala_KBM_MAP_OFFSET)]) {
                    entry = i;
                    break;
                }
            }
    
            out = oct * this->mtof_01_innerScala_kbmSize + entry + this->mtof_01_innerScala_kbmMid;
        }
    
        out = out + frac;
        this->mtof_01_innerScala_updateLast(out, hz);
        return this->mtof_01_innerScala_lastNote;
    }
    
    template<typename LISTTYPE = list> Int mtof_01_innerScala_updateScale(const LISTTYPE& scl) {
        if (scl->length < 1) {
            return 0;
        }
    
        number sclDataEntries = scl[0] * 2 + 1;
    
        if (sclDataEntries <= scl->length) {
            this->mtof_01_innerScala_lastValid = false;
            this->mtof_01_innerScala_sclExpMul = {};
            number last = 1;
    
            for (Index i = 1; i < sclDataEntries; i += 2) {
                const number c = (const number)(scl[(Index)(i + 0)]);
                const number d = (const number)(scl[(Index)(i + 1)]);
    
                if (d <= 0) {
                    last = c / (number)1200;
                } else {
                    last = rnbo_log2(c / d);
                }
    
                this->mtof_01_innerScala_sclExpMul->push(last);
            }
    
            this->mtof_01_innerScala_sclOctaveMul = last;
            this->mtof_01_innerScala_sclEntryCount = (Int)(this->mtof_01_innerScala_sclExpMul->length);
    
            if (scl->length >= sclDataEntries + 3) {
                this->mtof_01_innerScala_kbmMid = (Int)(scl[(Index)(sclDataEntries + 2)]);
                this->mtof_01_innerScala_kbmRefNum = (Int)(scl[(Index)(sclDataEntries + 1)]);
                this->mtof_01_innerScala_kbmRefFreq = scl[(Index)(sclDataEntries + 0)];
                this->mtof_01_innerScala_kbmSize = (Int)(0);
            }
    
            this->mtof_01_innerScala_updateRefFreq();
            return 1;
        }
    
        return 0;
    }
    
    template<typename LISTTYPE = list> Int mtof_01_innerScala_updateMap(const LISTTYPE& kbm) {
        list _kbm = kbm;
    
        if (_kbm->length == 1 && _kbm[0] == 0.0) {
            _kbm = {0.0, 0.0, 0.0, 60.0, 69.0, 440.0};
        }
    
        if (_kbm->length >= 6 && _kbm[0] >= 0.0) {
            this->mtof_01_innerScala_lastValid = false;
            Index size = (Index)(_kbm[0]);
            Int octave = 12;
    
            if (_kbm->length > 6) {
                octave = (Int)(_kbm[6]);
            }
    
            if (size > 0 && _kbm->length < this->mtof_01_innerScala_KBM_MAP_OFFSET) {
                return 0;
            }
    
            this->mtof_01_innerScala_kbmSize = (Int)(size);
            this->mtof_01_innerScala_kbmMin = (Int)(_kbm[1]);
            this->mtof_01_innerScala_kbmMax = (Int)(_kbm[2]);
            this->mtof_01_innerScala_kbmMid = (Int)(_kbm[3]);
            this->mtof_01_innerScala_kbmRefNum = (Int)(_kbm[4]);
            this->mtof_01_innerScala_kbmRefFreq = _kbm[5];
            this->mtof_01_innerScala_kbmOctaveDegree = octave;
            this->mtof_01_innerScala_kbmValid = _kbm;
            this->mtof_01_innerScala_kbmMapSize = (_kbm->length - this->mtof_01_innerScala_KBM_MAP_OFFSET > _kbm->length ? _kbm->length : (_kbm->length - this->mtof_01_innerScala_KBM_MAP_OFFSET < 0 ? 0 : _kbm->length - this->mtof_01_innerScala_KBM_MAP_OFFSET));
            this->mtof_01_innerScala_updateRefFreq();
            return 1;
        }
    
        return 0;
    }
    
    void mtof_01_innerScala_updateLast(number note, number freq) {
        this->mtof_01_innerScala_lastValid = true;
        this->mtof_01_innerScala_lastNote = note;
        this->mtof_01_innerScala_lastFreq = freq;
    }
    
    array<number, 2> mtof_01_innerScala_hztodeg(number hz) {
        number hza = rnbo_abs(hz);
    
        number octave = rnbo_floor(
            rnbo_log2(hza / this->mtof_01_innerScala_refFreq) / this->mtof_01_innerScala_sclOctaveMul
        );
    
        Int i = 0;
        number frac = 0;
        number n = 0;
    
        for (; i < this->mtof_01_innerScala_sclEntryCount; i++) {
            number c = this->mtof_01_innerScala_applySCLOctIndex(octave, i + 0, 0.0, this->mtof_01_innerScala_refFreq);
            n = this->mtof_01_innerScala_applySCLOctIndex(octave, i + 1, 0.0, this->mtof_01_innerScala_refFreq);
    
            if (c <= hza && hza < n) {
                if (c != hza) {
                    frac = rnbo_log2(hza / c) / rnbo_log2(n / c);
                }
    
                break;
            }
        }
    
        if (i == this->mtof_01_innerScala_sclEntryCount && n != hza) {
            number c = n;
            n = this->mtof_01_innerScala_applySCLOctIndex(octave + 1, 0, 0.0, this->mtof_01_innerScala_refFreq);
            frac = rnbo_log2(hza / c) / rnbo_log2(n / c);
        }
    
        number deg = i + octave * this->mtof_01_innerScala_sclEntryCount;
    
        {
            deg = rnbo_fround((deg + frac) * 1 / (number)1) * 1;
            frac = 0.0;
        }
    
        return {deg, frac};
    }
    
    array<Int, 2> mtof_01_innerScala_octdegree(Int degree, Int count) {
        Int octave = 0;
        Int index = 0;
    
        if (degree < 0) {
            octave = -(1 + (-1 - degree) / count);
            index = -degree % count;
    
            if (index > 0) {
                index = count - index;
            }
        } else {
            octave = degree / count;
            index = degree % count;
        }
    
        return {octave, index};
    }
    
    array<Int, 2> mtof_01_innerScala_applyKBM(number note) {
        if ((this->mtof_01_innerScala_kbmMin == this->mtof_01_innerScala_kbmMax && this->mtof_01_innerScala_kbmMax == 0) || (note >= this->mtof_01_innerScala_kbmMin && note <= this->mtof_01_innerScala_kbmMax)) {
            Int degree = (Int)(rnbo_floor(note - this->mtof_01_innerScala_kbmMid));
    
            if (this->mtof_01_innerScala_kbmSize == 0) {
                return {degree, 1};
            }
    
            array<Int, 2> octdeg = this->mtof_01_innerScala_octdegree(degree, this->mtof_01_innerScala_kbmSize);
            Int octave = (Int)(octdeg[0]);
            Index index = (Index)(octdeg[1]);
    
            if (this->mtof_01_innerScala_kbmMapSize > index) {
                degree = (Int)(this->mtof_01_innerScala_kbmValid[(Index)(this->mtof_01_innerScala_KBM_MAP_OFFSET + index)]);
    
                if (degree >= 0) {
                    return {degree + octave * this->mtof_01_innerScala_kbmOctaveDegree, 1};
                }
            }
        }
    
        return {-1, 0};
    }
    
    number mtof_01_innerScala_applySCL(Int degree, number frac, number refFreq) {
        array<Int, 2> octdeg = this->mtof_01_innerScala_octdegree(degree, this->mtof_01_innerScala_sclEntryCount);
        return this->mtof_01_innerScala_applySCLOctIndex(octdeg[0], octdeg[1], frac, refFreq);
    }
    
    number mtof_01_innerScala_applySCLOctIndex(number octave, Int index, number frac, number refFreq) {
        number p = 0;
    
        if (index > 0) {
            p = this->mtof_01_innerScala_sclExpMul[(Index)(index - 1)];
        }
    
        if (frac > 0) {
            p = this->linearinterp(frac, p, this->mtof_01_innerScala_sclExpMul[(Index)index]);
        } else if (frac < 0) {
            p = this->linearinterp(-frac, this->mtof_01_innerScala_sclExpMul[(Index)index], p);
        }
    
        return refFreq * rnbo_pow(2, p + octave * this->mtof_01_innerScala_sclOctaveMul);
    }
    
    void mtof_01_innerScala_updateRefFreq() {
        this->mtof_01_innerScala_lastValid = false;
        Int refOffset = (Int)(this->mtof_01_innerScala_kbmRefNum - this->mtof_01_innerScala_kbmMid);
    
        if (refOffset == 0) {
            this->mtof_01_innerScala_refFreq = this->mtof_01_innerScala_kbmRefFreq;
        } else {
            Int base = (Int)(this->mtof_01_innerScala_kbmSize);
    
            if (base < 1) {
                base = this->mtof_01_innerScala_sclEntryCount;
            }
    
            array<Int, 2> octdeg = this->mtof_01_innerScala_octdegree(refOffset, base);
            number oct = (number)(octdeg[0]);
            Int index = (Int)(octdeg[1]);
    
            if (base > 0) {
                oct = oct + rnbo_floor(index / base);
                index = index % base;
            }
    
            if (index >= 0 && index < this->mtof_01_innerScala_kbmSize) {
                if (index < (Int)(this->mtof_01_innerScala_kbmMapSize)) {
                    index = (Int)(this->mtof_01_innerScala_kbmValid[(Index)((Index)(index) + this->mtof_01_innerScala_KBM_MAP_OFFSET)]);
                } else {
                    index = -1;
                }
            }
    
            if (index < 0 || index > (Int)(this->mtof_01_innerScala_sclExpMul->length))
                {} else {
                number p = 0;
    
                if (index > 0) {
                    p = this->mtof_01_innerScala_sclExpMul[(Index)(index - 1)];
                }
    
                this->mtof_01_innerScala_refFreq = this->mtof_01_innerScala_kbmRefFreq / rnbo_pow(2, p + oct * this->mtof_01_innerScala_sclOctaveMul);
            }
        }
    }
    
    void mtof_01_innerScala_reset() {
        this->mtof_01_innerScala_lastValid = false;
        this->mtof_01_innerScala_lastNote = 0;
        this->mtof_01_innerScala_lastFreq = 0;
        this->mtof_01_innerScala_sclEntryCount = 0;
        this->mtof_01_innerScala_sclOctaveMul = 1;
        this->mtof_01_innerScala_sclExpMul = {};
        this->mtof_01_innerScala_kbmValid = {0, 0, 0, 60, 69, 440};
        this->mtof_01_innerScala_kbmMid = 60;
        this->mtof_01_innerScala_kbmRefNum = 69;
        this->mtof_01_innerScala_kbmRefFreq = 440;
        this->mtof_01_innerScala_kbmSize = 0;
        this->mtof_01_innerScala_kbmMin = 0;
        this->mtof_01_innerScala_kbmMax = 0;
        this->mtof_01_innerScala_kbmOctaveDegree = 12;
        this->mtof_01_innerScala_kbmMapSize = 0;
        this->mtof_01_innerScala_refFreq = 261.63;
    }
    
    void mtof_01_init() {
        this->mtof_01_innerScala_update(this->mtof_01_scale, this->mtof_01_map);
    }
    
    number cycle_tilde_01_ph_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->cycle_tilde_01_ph_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->cycle_tilde_01_ph_conv;
    
        if (this->cycle_tilde_01_ph_currentPhase < 0.)
            this->cycle_tilde_01_ph_currentPhase = 1. + this->cycle_tilde_01_ph_currentPhase;
    
        if (this->cycle_tilde_01_ph_currentPhase > 1.)
            this->cycle_tilde_01_ph_currentPhase = this->cycle_tilde_01_ph_currentPhase - 1.;
    
        number tmp = this->cycle_tilde_01_ph_currentPhase;
        this->cycle_tilde_01_ph_currentPhase += pincr;
        return tmp;
    }
    
    void cycle_tilde_01_ph_reset() {
        this->cycle_tilde_01_ph_currentPhase = 0;
    }
    
    void cycle_tilde_01_ph_dspsetup() {
        this->cycle_tilde_01_ph_conv = (number)1 / this->sr;
    }
    
    void cycle_tilde_01_dspsetup(bool force) {
        if ((bool)(this->cycle_tilde_01_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->cycle_tilde_01_phasei = 0;
        this->cycle_tilde_01_f2i = (number)4294967296 / this->sr;
        this->cycle_tilde_01_wrap = (Int)(this->cycle_tilde_01_buffer->getSize()) - 1;
        this->cycle_tilde_01_setupDone = true;
        this->cycle_tilde_01_ph_dspsetup();
    }
    
    void cycle_tilde_01_bufferUpdated() {
        this->cycle_tilde_01_wrap = (Int)(this->cycle_tilde_01_buffer->getSize()) - 1;
    }
    
    number cycle_tilde_02_ph_next(number freq, number reset) {
        {
            {
                if (reset >= 0.)
                    this->cycle_tilde_02_ph_currentPhase = reset;
            }
        }
    
        number pincr = freq * this->cycle_tilde_02_ph_conv;
    
        if (this->cycle_tilde_02_ph_currentPhase < 0.)
            this->cycle_tilde_02_ph_currentPhase = 1. + this->cycle_tilde_02_ph_currentPhase;
    
        if (this->cycle_tilde_02_ph_currentPhase > 1.)
            this->cycle_tilde_02_ph_currentPhase = this->cycle_tilde_02_ph_currentPhase - 1.;
    
        number tmp = this->cycle_tilde_02_ph_currentPhase;
        this->cycle_tilde_02_ph_currentPhase += pincr;
        return tmp;
    }
    
    void cycle_tilde_02_ph_reset() {
        this->cycle_tilde_02_ph_currentPhase = 0;
    }
    
    void cycle_tilde_02_ph_dspsetup() {
        this->cycle_tilde_02_ph_conv = (number)1 / this->sr;
    }
    
    void cycle_tilde_02_dspsetup(bool force) {
        if ((bool)(this->cycle_tilde_02_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->cycle_tilde_02_phasei = 0;
        this->cycle_tilde_02_f2i = (number)4294967296 / this->sr;
        this->cycle_tilde_02_wrap = (Int)(this->cycle_tilde_02_buffer->getSize()) - 1;
        this->cycle_tilde_02_setupDone = true;
        this->cycle_tilde_02_ph_dspsetup();
    }
    
    void cycle_tilde_02_bufferUpdated() {
        this->cycle_tilde_02_wrap = (Int)(this->cycle_tilde_02_buffer->getSize()) - 1;
    }
    
    void adsr_01_dspsetup(bool force) {
        if ((bool)(this->adsr_01_setupDone) && (bool)(!(bool)(force)))
            return;
    
        this->adsr_01_mspersamp = (number)1000 / this->sr;
        this->adsr_01_setupDone = true;
    }
    
    void param_01_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->param_01_value;
    }
    
    void param_01_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->param_01_value_set(preset["value"]);
    }
    
    void param_02_getPresetValue(PatcherStateInterface& preset) {
        preset["value"] = this->param_02_value;
    }
    
    void param_02_setPresetValue(PatcherStateInterface& preset) {
        if ((bool)(stateIsEmpty(preset)))
            return;
    
        this->param_02_value_set(preset["value"]);
    }
    
    void midiouthelper_sendMidi(number v) {
        this->midiouthelper_midiout_set(v);
    }
    
    bool stackprotect_check() {
        this->stackprotect_count++;
    
        if (this->stackprotect_count > 128) {
            console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
            return true;
        }
    
        return false;
    }
    
    Index getPatcherSerial() const {
        return 0;
    }
    
    void sendParameter(ParameterIndex index, bool ignoreValue) {
        if (this->_voiceIndex == 1)
            this->getPatcher()->sendParameter(index + this->parameterOffset, ignoreValue);
    }
    
    void scheduleParamInit(ParameterIndex index, Index order) {
        this->getPatcher()->scheduleParamInit(index + this->parameterOffset, order);
    }
    
    void updateTime(MillisecondTime time, EXTERNALENGINE* engine, bool inProcess = false) {
        RNBO_UNUSED(inProcess);
        RNBO_UNUSED(engine);
        this->_currentTime = time;
        auto offset = rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr));
    
        if (offset >= (SampleIndex)(this->vs))
            offset = (SampleIndex)(this->vs) - 1;
    
        if (offset < 0)
            offset = 0;
    
        this->sampleOffsetIntoNextAudioBuffer = (Index)(offset);
    }
    
    void assign_defaults()
    {
        mtof_01_base = 440;
        notein_01_channel = 0;
        dspexpr_01_in1 = 0;
        dspexpr_01_in2 = 0;
        cycle_tilde_01_frequency = 440;
        cycle_tilde_01_phase_offset = 0;
        cycle_tilde_02_frequency = 440;
        cycle_tilde_02_phase_offset = 0;
        dspexpr_02_in1 = 0;
        dspexpr_02_in2 = 0;
        expr_01_in1 = 0;
        expr_01_in2 = 127;
        expr_01_out1 = 0;
        dspexpr_03_in1 = 0;
        dspexpr_03_in2 = 0;
        dspexpr_04_in1 = 0;
        dspexpr_04_in2 = 0;
        adsr_01_trigger_number = 0;
        adsr_01_attack = 10;
        adsr_01_decay = 100;
        adsr_01_sustain = 0.8;
        adsr_01_release = 1000;
        adsr_01_legato = false;
        adsr_01_maxsustain = -1;
        param_01_value = 1;
        param_02_value = 0.1;
        _currentTime = 0;
        audioProcessSampleCount = 0;
        sampleOffsetIntoNextAudioBuffer = 0;
        zeroBuffer = nullptr;
        dummyBuffer = nullptr;
        signals[0] = nullptr;
        signals[1] = nullptr;
        signals[2] = nullptr;
        didAllocateSignals = 0;
        vs = 0;
        maxvs = 0;
        sr = 44100;
        invsr = 0.000022675736961451248;
        mtof_01_innerMtoF_lastInValue = 0;
        mtof_01_innerMtoF_lastOutValue = 0;
        mtof_01_innerMtoF_lastTuning = 0;
        mtof_01_innerScala_lastValid = false;
        mtof_01_innerScala_lastNote = 0;
        mtof_01_innerScala_lastFreq = 0;
        mtof_01_innerScala_sclEntryCount = 0;
        mtof_01_innerScala_sclOctaveMul = 1;
        mtof_01_innerScala_kbmValid = { 0, 0, 0, 60, 69, 440 };
        mtof_01_innerScala_kbmMid = 60;
        mtof_01_innerScala_kbmRefNum = 69;
        mtof_01_innerScala_kbmRefFreq = 440;
        mtof_01_innerScala_kbmSize = 0;
        mtof_01_innerScala_kbmMin = 0;
        mtof_01_innerScala_kbmMax = 0;
        mtof_01_innerScala_kbmOctaveDegree = 12;
        mtof_01_innerScala_kbmMapSize = 0;
        mtof_01_innerScala_refFreq = 261.63;
        notein_01_status = 0;
        notein_01_byte1 = -1;
        notein_01_inchan = 0;
        cycle_tilde_01_wrap = 0;
        cycle_tilde_01_ph_currentPhase = 0;
        cycle_tilde_01_ph_conv = 0;
        cycle_tilde_01_setupDone = false;
        cycle_tilde_02_wrap = 0;
        cycle_tilde_02_ph_currentPhase = 0;
        cycle_tilde_02_ph_conv = 0;
        cycle_tilde_02_setupDone = false;
        adsr_01_phase = 1;
        adsr_01_mspersamp = 0;
        adsr_01_time = 0;
        adsr_01_lastTriggerVal = 0;
        adsr_01_amplitude = 0;
        adsr_01_outval = 0;
        adsr_01_startingpoint = 0;
        adsr_01_triggerBuf = nullptr;
        adsr_01_triggerValueBuf = nullptr;
        adsr_01_setupDone = false;
        param_01_lastValue = 0;
        param_02_lastValue = 0;
        stackprotect_count = 0;
        _voiceIndex = 0;
        _noteNumber = 0;
        isMuted = 1;
        parameterOffset = 0;
    }
    
    // member variables
    
        list mtof_01_midivalue;
        list mtof_01_scale;
        list mtof_01_map;
        number mtof_01_base;
        number notein_01_channel;
        number dspexpr_01_in1;
        number dspexpr_01_in2;
        number cycle_tilde_01_frequency;
        number cycle_tilde_01_phase_offset;
        number cycle_tilde_02_frequency;
        number cycle_tilde_02_phase_offset;
        number dspexpr_02_in1;
        number dspexpr_02_in2;
        number expr_01_in1;
        number expr_01_in2;
        number expr_01_out1;
        number dspexpr_03_in1;
        number dspexpr_03_in2;
        number dspexpr_04_in1;
        number dspexpr_04_in2;
        number adsr_01_trigger_number;
        number adsr_01_attack;
        number adsr_01_decay;
        number adsr_01_sustain;
        number adsr_01_release;
        number adsr_01_legato;
        number adsr_01_maxsustain;
        number param_01_value;
        number param_02_value;
        MillisecondTime _currentTime;
        UInt64 audioProcessSampleCount;
        Index sampleOffsetIntoNextAudioBuffer;
        signal zeroBuffer;
        signal dummyBuffer;
        SampleValue * signals[3];
        bool didAllocateSignals;
        Index vs;
        Index maxvs;
        number sr;
        number invsr;
        number mtof_01_innerMtoF_lastInValue;
        number mtof_01_innerMtoF_lastOutValue;
        number mtof_01_innerMtoF_lastTuning;
        SampleBufferRef mtof_01_innerMtoF_buffer;
        const Index mtof_01_innerScala_KBM_MAP_OFFSET = 7;
        bool mtof_01_innerScala_lastValid;
        number mtof_01_innerScala_lastNote;
        number mtof_01_innerScala_lastFreq;
        Int mtof_01_innerScala_sclEntryCount;
        number mtof_01_innerScala_sclOctaveMul;
        list mtof_01_innerScala_sclExpMul;
        list mtof_01_innerScala_kbmValid;
        Int mtof_01_innerScala_kbmMid;
        Int mtof_01_innerScala_kbmRefNum;
        number mtof_01_innerScala_kbmRefFreq;
        Int mtof_01_innerScala_kbmSize;
        Int mtof_01_innerScala_kbmMin;
        Int mtof_01_innerScala_kbmMax;
        Int mtof_01_innerScala_kbmOctaveDegree;
        Index mtof_01_innerScala_kbmMapSize;
        number mtof_01_innerScala_refFreq;
        Int notein_01_status;
        Int notein_01_byte1;
        Int notein_01_inchan;
        SampleBufferRef cycle_tilde_01_buffer;
        Int cycle_tilde_01_wrap;
        UInt32 cycle_tilde_01_phasei;
        SampleValue cycle_tilde_01_f2i;
        number cycle_tilde_01_ph_currentPhase;
        number cycle_tilde_01_ph_conv;
        bool cycle_tilde_01_setupDone;
        SampleBufferRef cycle_tilde_02_buffer;
        Int cycle_tilde_02_wrap;
        UInt32 cycle_tilde_02_phasei;
        SampleValue cycle_tilde_02_f2i;
        number cycle_tilde_02_ph_currentPhase;
        number cycle_tilde_02_ph_conv;
        bool cycle_tilde_02_setupDone;
        Int adsr_01_phase;
        number adsr_01_mspersamp;
        number adsr_01_time;
        number adsr_01_lastTriggerVal;
        number adsr_01_amplitude;
        number adsr_01_outval;
        number adsr_01_startingpoint;
        signal adsr_01_triggerBuf;
        signal adsr_01_triggerValueBuf;
        bool adsr_01_setupDone;
        number param_01_lastValue;
        number param_02_lastValue;
        number stackprotect_count;
        Index _voiceIndex;
        Int _noteNumber;
        Index isMuted;
        ParameterIndex parameterOffset;
        bool _isInitialized = false;
};

		
void advanceTime(EXTERNALENGINE*) {}
void advanceTime(INTERNALENGINE*) {
	_internalEngine.advanceTime(sampstoms(this->vs));
}

void processInternalEvents(MillisecondTime time) {
	_internalEngine.processEventsUntil(time);
}

void updateTime(MillisecondTime time, INTERNALENGINE*, bool inProcess = false) {
	if (time == TimeNow) time = getPatcherTime();
	processInternalEvents(inProcess ? time + sampsToMs(this->vs) : time);
	updateTime(time, (EXTERNALENGINE*)nullptr);
}

rnbomatic* operator->() {
    return this;
}
const rnbomatic* operator->() const {
    return this;
}
rnbomatic* getTopLevelPatcher() {
    return this;
}

void cancelClockEvents()
{
}

template<typename LISTTYPE = list> void listquicksort(LISTTYPE& arr, LISTTYPE& sortindices, Int l, Int h, bool ascending) {
    if (l < h) {
        Int p = (Int)(this->listpartition(arr, sortindices, l, h, ascending));
        this->listquicksort(arr, sortindices, l, p - 1, ascending);
        this->listquicksort(arr, sortindices, p + 1, h, ascending);
    }
}

template<typename LISTTYPE = list> Int listpartition(LISTTYPE& arr, LISTTYPE& sortindices, Int l, Int h, bool ascending) {
    number x = arr[(Index)h];
    Int i = (Int)(l - 1);

    for (Int j = (Int)(l); j <= h - 1; j++) {
        bool asc = (bool)((bool)(ascending) && arr[(Index)j] <= x);
        bool desc = (bool)((bool)(!(bool)(ascending)) && arr[(Index)j] >= x);

        if ((bool)(asc) || (bool)(desc)) {
            i++;
            this->listswapelements(arr, i, j);
            this->listswapelements(sortindices, i, j);
        }
    }

    i++;
    this->listswapelements(arr, i, h);
    this->listswapelements(sortindices, i, h);
    return i;
}

template<typename LISTTYPE = list> void listswapelements(LISTTYPE& arr, Int a, Int b) {
    auto tmp = arr[(Index)a];
    arr[(Index)a] = arr[(Index)b];
    arr[(Index)b] = tmp;
}

number mstosamps(MillisecondTime ms) {
    return ms * this->sr * 0.001;
}

number maximum(number x, number y) {
    return (x < y ? y : x);
}

MillisecondTime sampstoms(number samps) {
    return samps * 1000 / this->sr;
}

void param_03_value_set(number v) {
    v = this->param_03_value_constrain(v);
    this->param_03_value = v;
    this->sendParameter(0, false);

    if (this->param_03_value != this->param_03_lastValue) {
        this->getEngine()->presetTouched();
        this->param_03_lastValue = this->param_03_value;
    }

    this->poly_index_set(v);
}

void param_04_value_set(number v) {
    v = this->param_04_value_constrain(v);
    this->param_04_value = v;
    this->sendParameter(1, false);

    if (this->param_04_value != this->param_04_lastValue) {
        this->getEngine()->presetTouched();
        this->param_04_lastValue = this->param_04_value;
    }

    this->poly_volume_set(v);
}

MillisecondTime getPatcherTime() const {
    return this->_currentTime;
}

void deallocateSignals() {
    Index i;
    this->globaltransport_tempo = freeSignal(this->globaltransport_tempo);
    this->globaltransport_state = freeSignal(this->globaltransport_state);
    this->zeroBuffer = freeSignal(this->zeroBuffer);
    this->dummyBuffer = freeSignal(this->dummyBuffer);
}

Index getMaxBlockSize() const {
    return this->maxvs;
}

number getSampleRate() const {
    return this->sr;
}

bool hasFixedVectorSize() const {
    return false;
}

void setProbingTarget(MessageTag ) {}

void fillRNBODefaultMtofLookupTable256(DataRef& ref) {
    SampleBuffer buffer(ref);
    number bufsize = buffer->getSize();

    for (Index i = 0; i < bufsize; i++) {
        number midivalue = -256. + (number)512. / (bufsize - 1) * i;
        buffer[i] = rnbo_exp(.057762265 * (midivalue - 69.0));
    }
}

void fillRNBODefaultSinus(DataRef& ref) {
    SampleBuffer buffer(ref);
    number bufsize = buffer->getSize();

    for (Index i = 0; i < bufsize; i++) {
        buffer[i] = rnbo_cos(i * 3.14159265358979323846 * 2. / bufsize);
    }
}

void fillDataRef(DataRefIndex index, DataRef& ref) {
    switch (index) {
    case 0:
        {
        this->fillRNBODefaultMtofLookupTable256(ref);
        break;
        }
    case 1:
        {
        this->fillRNBODefaultSinus(ref);
        break;
        }
    }
}

void allocateDataRefs() {
    for (Index i = 0; i < 16; i++) {
        this->poly[i]->allocateDataRefs();
    }

    if (this->RNBODefaultMtofLookupTable256->hasRequestedSize()) {
        if (this->RNBODefaultMtofLookupTable256->wantsFill())
            this->fillRNBODefaultMtofLookupTable256(this->RNBODefaultMtofLookupTable256);

        this->getEngine()->sendDataRefUpdated(0);
    }

    if (this->RNBODefaultSinus->hasRequestedSize()) {
        if (this->RNBODefaultSinus->wantsFill())
            this->fillRNBODefaultSinus(this->RNBODefaultSinus);

        this->getEngine()->sendDataRefUpdated(1);
    }
}

void initializeObjects() {
    this->midinotecontroller_01_init();

    for (Index i = 0; i < 16; i++) {
        this->poly[i]->initializeObjects();
    }
}

Index getIsMuted()  {
    return this->isMuted;
}

void setIsMuted(Index v)  {
    this->isMuted = v;
}

void onSampleRateChanged(double ) {}

void extractState(PatcherStateInterface& ) {}

void applyState() {
    for (Index i = 0; i < 16; i++) {

        this->poly[(Index)i]->setEngineAndPatcher(this->getEngine(), this);
        this->poly[(Index)i]->initialize();
        this->poly[(Index)i]->setParameterOffset(this->getParameterOffset(this->poly[0]));
        this->poly[(Index)i]->setVoiceIndex(i + 1);
    }
}

ParameterIndex getParameterOffset(BaseInterface& subpatcher) const {
    if (addressOf(subpatcher) == addressOf(this->poly[0]))
        return 2;

    return 0;
}

void processClockEvent(MillisecondTime , ClockId , bool , ParameterValue ) {}

void processOutletAtCurrentTime(EngineLink* , OutletIndex , ParameterValue ) {}

void processOutletEvent(
    EngineLink* sender,
    OutletIndex index,
    ParameterValue value,
    MillisecondTime time
) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->processOutletAtCurrentTime(sender, index, value);
}

void sendOutlet(OutletIndex index, ParameterValue value) {
    this->getEngine()->sendOutlet(this, index, value);
}

void startup() {
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);

    for (Index i = 0; i < 16; i++) {
        this->poly[i]->startup();
    }

    {
        this->scheduleParamInit(0, 0);
    }

    {
        this->scheduleParamInit(1, 0);
    }

    this->processParamInitEvents();
}

number param_03_value_constrain(number v) const {
    v = (v > 7 ? 7 : (v < 1 ? 1 : v));
    return v;
}

void poly_index_set(number v) {
    for (number i = 0; i < 16; i++) {
        if (i + 1 == this->poly_target || 0 == this->poly_target) {
            this->poly[(Index)i]->setParameterValue(0, v, this->_currentTime);
        }
    }
}

number param_04_value_constrain(number v) const {
    v = (v > 1 ? 1 : (v < 0 ? 0 : v));
    return v;
}

void poly_volume_set(number v) {
    for (number i = 0; i < 16; i++) {
        if (i + 1 == this->poly_target || 0 == this->poly_target) {
            this->poly[(Index)i]->setParameterValue(1, v, this->_currentTime);
        }
    }
}

void midinotecontroller_01_currenttarget_set(number v) {
    this->midinotecontroller_01_currenttarget = v;
}

void poly_target_set(number v) {
    this->poly_target = v;
    this->midinotecontroller_01_currenttarget_set(v);
}

void midinotecontroller_01_target_set(number v) {
    this->poly_target_set(v);
}

void poly_midiininternal_set(number v) {
    Index sendlen = 0;
    this->poly_currentStatus = parseMidi(this->poly_currentStatus, (Int)(v), this->poly_mididata[0]);

    switch ((int)this->poly_currentStatus) {
    case MIDI_StatusByteReceived:
        {
        this->poly_mididata[0] = (uint8_t)(v);
        this->poly_mididata[1] = 0;
        break;
        }
    case MIDI_SecondByteReceived:
    case MIDI_ProgramChange:
    case MIDI_ChannelPressure:
        {
        this->poly_mididata[1] = (uint8_t)(v);

        if (this->poly_currentStatus == MIDI_ProgramChange || this->poly_currentStatus == MIDI_ChannelPressure) {
            sendlen = 2;
        }

        break;
        }
    case MIDI_NoteOff:
    case MIDI_NoteOn:
    case MIDI_Aftertouch:
    case MIDI_CC:
    case MIDI_PitchBend:
    default:
        {
        this->poly_mididata[2] = (uint8_t)(v);
        sendlen = 3;
        break;
        }
    }

    if (sendlen > 0) {
        number i;

        if (this->poly_target > 0 && this->poly_target <= 16) {
            i = this->poly_target - 1;
            this->poly[(Index)i]->processMidiEvent(_currentTime, 0, this->poly_mididata, sendlen);
        } else if (this->poly_target == 0) {
            for (i = 0; i < 16; i++) {
                this->poly[(Index)i]->processMidiEvent(_currentTime, 0, this->poly_mididata, sendlen);
            }
        }
    }
}

void midinotecontroller_01_midiout_set(number v) {
    this->poly_midiininternal_set(v);
}

void poly_noteNumber_set(number v) {
    if (this->poly_target > 0) {
        this->poly[(Index)(this->poly_target - 1)]->setNoteNumber((Int)(v));
    }
}

void midinotecontroller_01_noteNumber_set(number v) {
    this->poly_noteNumber_set(v);
}

template<typename LISTTYPE> void midinotecontroller_01_voicestatus_set(const LISTTYPE& v) {
    if (v[1] == 1) {
        number currentTarget = this->midinotecontroller_01_currenttarget;
        this->midinotecontroller_01_target_set(v[0]);
        this->midinotecontroller_01_noteNumber_set(0);
        this->midinotecontroller_01_target_set(currentTarget);
    }
}

template<typename LISTTYPE> void poly_voicestatus_set(const LISTTYPE& v) {
    this->midinotecontroller_01_voicestatus_set(v);
}

void poly_activevoices_set(number ) {}

template<typename LISTTYPE> void poly_mute_set(const LISTTYPE& v) {
    Index voiceNumber = (Index)(v[0]);
    Index muteState = (Index)(v[1]);

    if (voiceNumber == 0) {
        for (Index i = 0; i < 16; i++) {
            this->poly[(Index)i]->setIsMuted(muteState);
        }
    } else {
        Index subpatcherIndex = voiceNumber - 1;

        if (subpatcherIndex >= 0 && subpatcherIndex < 16) {
            this->poly[(Index)subpatcherIndex]->setIsMuted(muteState);
        }
    }

    list tmp = {v[0], v[1]};
    this->poly_voicestatus_set(tmp);
    this->poly_activevoices_set(this->poly_calcActiveVoices());
}

template<typename LISTTYPE> void midinotecontroller_01_mute_set(const LISTTYPE& v) {
    this->poly_mute_set(v);
}

void midinotecontroller_01_midiin_set(number v) {
    this->midinotecontroller_01_midiin = v;
    Int val = (Int)(v);

    this->midinotecontroller_01_currentStatus = parseMidi(
        this->midinotecontroller_01_currentStatus,
        (Int)(v),
        this->midinotecontroller_01_status
    );

    switch ((int)this->midinotecontroller_01_currentStatus) {
    case MIDI_StatusByteReceived:
        {
        {
            this->midinotecontroller_01_status = val;
            this->midinotecontroller_01_byte1 = -1;
            break;
        }
        }
    case MIDI_SecondByteReceived:
        {
        this->midinotecontroller_01_byte1 = val;
        break;
        }
    case MIDI_NoteOn:
        {
        {
            bool sendnoteoff = true;
            Int target = 1;
            MillisecondTime oldest = (MillisecondTime)(this->midinotecontroller_01_voice_lastontime[0]);
            number target_state = this->midinotecontroller_01_voice_state[0];

            for (Int i = 0; i < 16; i++) {
                number candidate_state = this->midinotecontroller_01_voice_state[(Index)i];

                if (this->midinotecontroller_01_voice_notenumber[(Index)i] == this->midinotecontroller_01_byte1 && candidate_state == MIDI_NoteState_On) {
                    sendnoteoff = false;
                    target = i + 1;
                    break;
                }

                if (i > 0) {
                    if (candidate_state != MIDI_NoteState_On || target_state == MIDI_NoteState_On) {
                        MillisecondTime candidate_ontime = (MillisecondTime)(this->midinotecontroller_01_voice_lastontime[(Index)i]);

                        if (candidate_ontime < oldest || (target_state == MIDI_NoteState_On && candidate_state != MIDI_NoteState_On)) {
                            target = i + 1;
                            oldest = candidate_ontime;
                            target_state = candidate_state;
                        }
                    }
                }
            }

            if ((bool)(sendnoteoff))
                this->midinotecontroller_01_sendnoteoff(target);

            Int i = (Int)(target - 1);
            this->midinotecontroller_01_voice_state[(Index)i] = MIDI_NoteState_On;
            this->midinotecontroller_01_voice_lastontime[(Index)i] = this->_currentTime;
            this->midinotecontroller_01_voice_notenumber[(Index)i] = this->midinotecontroller_01_byte1;
            this->midinotecontroller_01_voice_channel[(Index)i] = (BinOpInt)((BinOpInt)this->midinotecontroller_01_status & (BinOpInt)0x0F);

            for (Index j = 0; j < 128; j++) {
                if (this->midinotecontroller_01_notesdown[(Index)j] == 0) {
                    this->midinotecontroller_01_notesdown[(Index)j] = this->midinotecontroller_01_voice_notenumber[(Index)i];
                    break;
                }
            }

            this->midinotecontroller_01_note_lastvelocity[(Index)this->midinotecontroller_01_voice_notenumber[(Index)i]] = val;
            this->midinotecontroller_01_sendpitchbend(i);
            this->midinotecontroller_01_sendpressure(i);
            this->midinotecontroller_01_sendtimbre(i);
            this->midinotecontroller_01_muteval[0] = target;
            this->midinotecontroller_01_muteval[1] = 0;
            this->midinotecontroller_01_mute_set(this->midinotecontroller_01_muteval);
            number currentTarget = this->midinotecontroller_01_currenttarget;
            this->midinotecontroller_01_target_set(target);
            this->midinotecontroller_01_noteNumber_set(this->midinotecontroller_01_voice_notenumber[(Index)i]);

            this->midinotecontroller_01_midiout_set(
                (BinOpInt)((BinOpInt)MIDI_NoteOnMask | (BinOpInt)this->midinotecontroller_01_voice_channel[(Index)i])
            );

            this->midinotecontroller_01_midiout_set(this->midinotecontroller_01_voice_notenumber[(Index)i]);
            this->midinotecontroller_01_midiout_set(val);
            this->midinotecontroller_01_target_set(currentTarget);
            break;
        }
        }
    case MIDI_NoteOff:
        {
        {
            Int target = 0;
            number notenumber = this->midinotecontroller_01_byte1;
            number channel = (BinOpInt)((BinOpInt)this->midinotecontroller_01_status & (BinOpInt)0x0F);

            for (Int i = 0; i < 16; i++) {
                if (this->midinotecontroller_01_voice_notenumber[(Index)i] == notenumber && this->midinotecontroller_01_voice_channel[(Index)i] == channel && this->midinotecontroller_01_voice_state[(Index)i] == MIDI_NoteState_On) {
                    target = i + 1;
                    break;
                }
            }

            if (target > 0) {
                Int i = (Int)(target - 1);
                Index j = (Index)(channel);
                bool ignoresustainchannel = true;

                if ((bool)(this->midinotecontroller_01_channel_sustain[((bool)(ignoresustainchannel) ? 0 : j)])) {
                    this->midinotecontroller_01_voice_state[(Index)i] = MIDI_NoteState_Sustained;
                } else {
                    number currentTarget = this->midinotecontroller_01_currenttarget;
                    this->midinotecontroller_01_target_set(target);
                    this->midinotecontroller_01_midiout_set(this->midinotecontroller_01_status);
                    this->midinotecontroller_01_midiout_set(this->midinotecontroller_01_byte1);
                    this->midinotecontroller_01_midiout_set(v);
                    this->midinotecontroller_01_target_set(currentTarget);

                    if (this->midinotecontroller_01_currentStatus == MIDI_NoteOff) {
                        this->midinotecontroller_01_voice_state[(Index)i] = MIDI_NoteState_Off;
                    }
                }
            } else
                {}

            bool found = false;

            for (Index i = 0; i < 128; i++) {
                if (this->midinotecontroller_01_notesdown[(Index)i] == 0) {
                    break;
                } else if (this->midinotecontroller_01_notesdown[(Index)i] == notenumber) {
                    found = true;
                }

                if ((bool)(found)) {
                    this->midinotecontroller_01_notesdown[(Index)i] = this->midinotecontroller_01_notesdown[(Index)(i + 1)];
                }
            }

            break;
        }
        }
    case MIDI_Aftertouch:
        {
        {
            number currentTarget = this->midinotecontroller_01_currenttarget;
            this->midinotecontroller_01_target_set(0);
            this->midinotecontroller_01_midiout_set(this->midinotecontroller_01_status);
            this->midinotecontroller_01_midiout_set(this->midinotecontroller_01_byte1);
            this->midinotecontroller_01_midiout_set(v);
            this->midinotecontroller_01_target_set(currentTarget);
            break;
        }
        }
    case MIDI_CC:
        {
        {
            bool sendToAllVoices = true;

            switch ((int)this->midinotecontroller_01_byte1) {
            case MIDI_CC_Sustain:
                {
                {
                    bool pedaldown = (bool)((val >= 64 ? true : false));
                    number channel = (BinOpInt)((BinOpInt)this->midinotecontroller_01_status & (BinOpInt)0x0F);
                    Index j = (Index)(channel);
                    bool ignoresustainchannel = true;
                    this->midinotecontroller_01_channel_sustain[((bool)(ignoresustainchannel) ? 0 : j)] = pedaldown;

                    if ((bool)(!(bool)(pedaldown))) {
                        for (Index i = 0; i < 16; i++) {
                            if (((bool)(ignoresustainchannel) || this->midinotecontroller_01_voice_channel[(Index)i] == channel) && this->midinotecontroller_01_voice_state[(Index)i] == MIDI_NoteState_Sustained) {
                                number currentTarget = this->midinotecontroller_01_currenttarget;
                                this->midinotecontroller_01_target_set(i + 1);
                                this->midinotecontroller_01_midiout_set((BinOpInt)((BinOpInt)MIDI_NoteOffMask | (BinOpInt)j));
                                this->midinotecontroller_01_midiout_set(this->midinotecontroller_01_voice_notenumber[(Index)i]);
                                this->midinotecontroller_01_midiout_set(64);
                                this->midinotecontroller_01_target_set(currentTarget);
                                this->midinotecontroller_01_voice_state[(Index)i] = MIDI_NoteState_Off;
                            }
                        }
                    }

                    break;
                }
                }
            case MIDI_CC_TimbreMSB:
                {
                {
                    number channel = (BinOpInt)((BinOpInt)this->midinotecontroller_01_status & (BinOpInt)0x0F);
                    Int k = (Int)(v);
                    number timbre = (BinOpInt)(((BinOpInt)((BinOpInt)k & (BinOpInt)0x7F)) << imod_nocast((UBinOpInt)7, 32));
                    this->midinotecontroller_01_channel_timbre[(Index)((BinOpInt)this->midinotecontroller_01_status & (BinOpInt)0x0F)] = timbre;

                    for (Int i = 0; i < 16; i++) {
                        if (this->midinotecontroller_01_voice_channel[(Index)i] == channel && this->midinotecontroller_01_voice_state[(Index)i] != MIDI_NoteState_Off) {
                            this->midinotecontroller_01_sendtimbre(i);
                        }
                    }

                    sendToAllVoices = false;
                    break;
                }
                }
            case MIDI_CC_TimbreLSB:
                {
                {
                    break;
                }
                }
            case MIDI_CC_AllNotesOff:
                {
                {
                    this->midinotecontroller_01_sendallnotesoff();
                    break;
                }
                }
            }

            if ((bool)(sendToAllVoices)) {
                number currentTarget = this->midinotecontroller_01_currenttarget;
                this->midinotecontroller_01_target_set(0);
                this->midinotecontroller_01_midiout_set(this->midinotecontroller_01_status);
                this->midinotecontroller_01_midiout_set(this->midinotecontroller_01_byte1);
                this->midinotecontroller_01_midiout_set(v);
                this->midinotecontroller_01_target_set(currentTarget);
            }

            break;
        }
        }
    case MIDI_ProgramChange:
        {
        {
            number currentTarget = this->midinotecontroller_01_currenttarget;
            this->midinotecontroller_01_target_set(0);
            this->midinotecontroller_01_midiout_set(this->midinotecontroller_01_status);
            this->midinotecontroller_01_midiout_set(this->midinotecontroller_01_byte1);
            this->midinotecontroller_01_target_set(currentTarget);
            break;
        }
        }
    case MIDI_ChannelPressure:
        {
        {
            number channel = (BinOpInt)((BinOpInt)this->midinotecontroller_01_status & (BinOpInt)0x0F);

            for (Int i = 0; i < 16; i++) {
                if (this->midinotecontroller_01_voice_state[(Index)i] != MIDI_NoteState_Off && this->midinotecontroller_01_voice_channel[(Index)i] == channel) {
                    Int k = (Int)(channel);
                    this->midinotecontroller_01_channel_pressure[(Index)k] = v;
                    this->midinotecontroller_01_sendpressure(i);
                }
            }

            break;
        }
        }
    case MIDI_PitchBend:
        {
        {
            number bendamount = (BinOpInt)((BinOpInt)this->midinotecontroller_01_byte1 | (BinOpInt)((BinOpInt)val << imod_nocast((UBinOpInt)7, 32)));
            Int channel = (Int)((BinOpInt)((BinOpInt)this->midinotecontroller_01_status & (BinOpInt)0x0F));
            this->midinotecontroller_01_channel_pitchbend[(Index)channel] = bendamount;

            for (Int i = 0; i < 16; i++) {
                if (this->midinotecontroller_01_voice_state[(Index)i] != MIDI_NoteState_Off && this->midinotecontroller_01_voice_channel[(Index)i] == channel) {
                    this->midinotecontroller_01_sendpitchbend(i);
                }
            }

            break;
        }
        }
    }
}

void poly_midiin_set(number v) {
    this->poly_midiin = v;
    this->midinotecontroller_01_midiin_set(v);
}

void midiin_midiout_set(number v) {
    this->poly_midiin_set(v);
}

void midiin_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(port);
    RNBO_UNUSED(channel);
    RNBO_UNUSED(status);
    Index i;

    for (i = 0; i < length; i++) {
        this->midiin_midiout_set(data[i]);
    }
}

template<typename LISTTYPE> void eventoutlet_02_in1_list_set(const LISTTYPE& v) {
    this->getEngine()->sendListMessage(TAG("out3"), TAG(""), v, this->_currentTime);
}

template<typename LISTTYPE> void poly_out3_list_set(const LISTTYPE& v) {
    this->eventoutlet_02_in1_list_set(v);
}

void poly_perform(SampleValue * out1, SampleValue * out2, Index n) {
    SampleArray<2> outs = {out1, out2};

    for (number chan = 0; chan < 2; chan++)
        zeroSignal(outs[(Index)chan], n);

    for (Index i = 0; i < 16; i++)
        this->poly[(Index)i]->process(nullptr, 0, outs, 2, n);
}

void stackprotect_perform(Index n) {
    RNBO_UNUSED(n);
    auto __stackprotect_count = this->stackprotect_count;
    __stackprotect_count = 0;
    this->stackprotect_count = __stackprotect_count;
}

void param_03_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_03_value;
}

void param_03_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_03_value_set(preset["value"]);
}

void param_04_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_04_value;
}

void param_04_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_04_value_set(preset["value"]);
}

number poly_calcActiveVoices() {
    {
        number activeVoices = 0;

        for (Index i = 0; i < 16; i++) {
            if ((bool)(!(bool)(this->poly[(Index)i]->getIsMuted())))
                activeVoices++;
        }

        return activeVoices;
    }
}

void midinotecontroller_01_init() {
    for (Index i = 0; i < 16; i++) {
        this->midinotecontroller_01_channel_pitchbend[(Index)i] = 0x2000;
    }

    for (Index i = 0; i < 16; i++) {
        this->midinotecontroller_01_voice_lastontime[(Index)i] = -1;
    }
}

void midinotecontroller_01_sendnoteoff(Int target) {
    Int i = (Int)(target - 1);

    if (this->midinotecontroller_01_voice_state[(Index)i] != MIDI_NoteState_Off) {
        number currentTarget = this->midinotecontroller_01_currenttarget;
        this->midinotecontroller_01_target_set(target);

        this->midinotecontroller_01_midiout_set(
            (BinOpInt)((BinOpInt)MIDI_NoteOffMask | (BinOpInt)this->midinotecontroller_01_voice_channel[(Index)i])
        );

        this->midinotecontroller_01_midiout_set(this->midinotecontroller_01_voice_notenumber[(Index)i]);
        this->midinotecontroller_01_midiout_set(64);
        this->midinotecontroller_01_voice_state[(Index)i] = MIDI_NoteState_Off;
        this->midinotecontroller_01_target_set(currentTarget);
    }
}

void midinotecontroller_01_sendpitchbend(Int v) {
    if (v >= 0 && v < 16) {
        number currentTarget = this->midinotecontroller_01_currenttarget;
        this->midinotecontroller_01_target_set(v + 1);
        Int totalbendamount = (Int)(this->midinotecontroller_01_channel_pitchbend[(Index)this->midinotecontroller_01_voice_channel[(Index)v]]);

        this->midinotecontroller_01_midiout_set(
            (BinOpInt)((BinOpInt)MIDI_PitchBendMask | (BinOpInt)this->midinotecontroller_01_voice_channel[(Index)v])
        );

        this->midinotecontroller_01_midiout_set((BinOpInt)((BinOpInt)totalbendamount & (BinOpInt)0x7F));

        this->midinotecontroller_01_midiout_set(
            (BinOpInt)((BinOpInt)((BinOpInt)totalbendamount >> imod_nocast((UBinOpInt)imod_nocast((UBinOpInt)7, 32), 32)) & (BinOpInt)0x7F)
        );

        this->midinotecontroller_01_target_set(currentTarget);
    }
}

void midinotecontroller_01_sendpressure(Int v) {
    number currentTarget = this->midinotecontroller_01_currenttarget;
    this->midinotecontroller_01_target_set(v + 1);

    this->midinotecontroller_01_midiout_set(
        (BinOpInt)((BinOpInt)MIDI_ChannelPressureMask | (BinOpInt)this->midinotecontroller_01_voice_channel[(Index)v])
    );

    this->midinotecontroller_01_midiout_set(
        this->midinotecontroller_01_channel_pressure[(Index)this->midinotecontroller_01_voice_channel[(Index)v]]
    );

    this->midinotecontroller_01_target_set(currentTarget);
}

void midinotecontroller_01_sendtimbre(Int v) {
    number currentTarget = this->midinotecontroller_01_currenttarget;
    this->midinotecontroller_01_target_set(v + 1);

    this->midinotecontroller_01_midiout_set(
        (BinOpInt)((BinOpInt)MIDI_CCMask | (BinOpInt)this->midinotecontroller_01_voice_channel[(Index)v])
    );

    this->midinotecontroller_01_midiout_set(MIDI_CC_TimbreLSB);

    this->midinotecontroller_01_midiout_set(
        (BinOpInt)((BinOpInt)this->midinotecontroller_01_channel_timbre[(Index)this->midinotecontroller_01_voice_channel[(Index)v]] & (BinOpInt)0x7F)
    );

    this->midinotecontroller_01_midiout_set(
        (BinOpInt)((BinOpInt)MIDI_CCMask | (BinOpInt)this->midinotecontroller_01_voice_channel[(Index)v])
    );

    this->midinotecontroller_01_midiout_set(MIDI_CC_TimbreMSB);

    this->midinotecontroller_01_midiout_set(
        (BinOpInt)((BinOpInt)((BinOpInt)this->midinotecontroller_01_channel_timbre[(Index)this->midinotecontroller_01_voice_channel[(Index)v]] >> imod_nocast((UBinOpInt)7, 32)) & (BinOpInt)0x7F)
    );

    this->midinotecontroller_01_target_set(currentTarget);
}

void midinotecontroller_01_sendallnotesoff() {
    for (Int i = 1; i <= 16; i++) {
        this->midinotecontroller_01_sendnoteoff(i);
    }
}

void globaltransport_advance() {}

void globaltransport_dspsetup(bool ) {}

bool stackprotect_check() {
    this->stackprotect_count++;

    if (this->stackprotect_count > 128) {
        console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
        return true;
    }

    return false;
}

Index getPatcherSerial() const {
    return 0;
}

void sendParameter(ParameterIndex index, bool ignoreValue) {
    this->getEngine()->notifyParameterValueChanged(index, (ignoreValue ? 0 : this->getParameterValue(index)), ignoreValue);
}

void scheduleParamInit(ParameterIndex index, Index order) {
    this->paramInitIndices->push(index);
    this->paramInitOrder->push(order);
}

void processParamInitEvents() {
    this->listquicksort(
        this->paramInitOrder,
        this->paramInitIndices,
        0,
        (int)(this->paramInitOrder->length - 1),
        true
    );

    for (Index i = 0; i < this->paramInitOrder->length; i++) {
        this->getEngine()->scheduleParameterBang(this->paramInitIndices[i], 0);
    }
}

void updateTime(MillisecondTime time, EXTERNALENGINE* engine, bool inProcess = false) {
    RNBO_UNUSED(inProcess);
    RNBO_UNUSED(engine);
    this->_currentTime = time;
    auto offset = rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr));

    if (offset >= (SampleIndex)(this->vs))
        offset = (SampleIndex)(this->vs) - 1;

    if (offset < 0)
        offset = 0;

    this->sampleOffsetIntoNextAudioBuffer = (Index)(offset);
}

void assign_defaults()
{
    midiin_port = 0;
    midiout_port = 0;
    param_03_value = 1;
    param_04_value = 0.1;
    poly_target = 0;
    poly_midiin = 0;
    midinotecontroller_01_currenttarget = 0;
    midinotecontroller_01_midiin = 0;
    _currentTime = 0;
    audioProcessSampleCount = 0;
    sampleOffsetIntoNextAudioBuffer = 0;
    zeroBuffer = nullptr;
    dummyBuffer = nullptr;
    didAllocateSignals = 0;
    vs = 0;
    maxvs = 0;
    sr = 44100;
    invsr = 0.000022675736961451248;
    midiout_currentStatus = -1;
    midiout_status = -1;
    midiout_byte1 = -1;
    param_03_lastValue = 0;
    param_04_lastValue = 0;
    poly_currentStatus = -1;
    poly_mididata[0] = 0;
    poly_mididata[1] = 0;
    poly_mididata[2] = 0;
    midinotecontroller_01_currentStatus = -1;
    midinotecontroller_01_status = -1;
    midinotecontroller_01_byte1 = -1;
    midinotecontroller_01_zone_masterchannel = 1;
    midinotecontroller_01_zone_numnotechannels = 15;
    midinotecontroller_01_zone_masterpitchbendrange = 2;
    midinotecontroller_01_zone_pernotepitchbendrange = 48;
    midinotecontroller_01_muteval = { 0, 0 };
    globaltransport_tempo = nullptr;
    globaltransport_state = nullptr;
    stackprotect_count = 0;
    _voiceIndex = 0;
    _noteNumber = 0;
    isMuted = 1;
}

    // data ref strings
    struct DataRefStrings {
    	static constexpr auto& name0 = "RNBODefaultMtofLookupTable256";
    	static constexpr auto& file0 = "";
    	static constexpr auto& tag0 = "buffer~";
    	static constexpr auto& name1 = "RNBODefaultSinus";
    	static constexpr auto& file1 = "";
    	static constexpr auto& tag1 = "buffer~";
    	DataRefStrings* operator->() { return this; }
    	const DataRefStrings* operator->() const { return this; }
    };

    DataRefStrings dataRefStrings;

// member variables

    number midiin_port;
    number midiout_port;
    number param_03_value;
    number param_04_value;
    number poly_target;
    number poly_midiin;
    number midinotecontroller_01_currenttarget;
    number midinotecontroller_01_midiin;
    MillisecondTime _currentTime;
    ENGINE _internalEngine;
    UInt64 audioProcessSampleCount;
    Index sampleOffsetIntoNextAudioBuffer;
    signal zeroBuffer;
    signal dummyBuffer;
    bool didAllocateSignals;
    Index vs;
    Index maxvs;
    number sr;
    number invsr;
    Int midiout_currentStatus;
    Int midiout_status;
    Int midiout_byte1;
    list midiout_sysex;
    number param_03_lastValue;
    number param_04_lastValue;
    Int poly_currentStatus;
    uint8_t poly_mididata[3];
    Int midinotecontroller_01_currentStatus;
    Int midinotecontroller_01_status;
    Int midinotecontroller_01_byte1;
    Int midinotecontroller_01_zone_masterchannel;
    Int midinotecontroller_01_zone_numnotechannels;
    Int midinotecontroller_01_zone_masterpitchbendrange;
    Int midinotecontroller_01_zone_pernotepitchbendrange;
    number midinotecontroller_01_channel_pitchbend[16] = { };
    number midinotecontroller_01_channel_pressure[16] = { };
    number midinotecontroller_01_channel_timbre[16] = { };
    Int midinotecontroller_01_channel_sustain[16] = { };
    MillisecondTime midinotecontroller_01_voice_lastontime[16] = { };
    number midinotecontroller_01_voice_notenumber[16] = { };
    number midinotecontroller_01_voice_channel[16] = { };
    number midinotecontroller_01_voice_state[16] = { };
    number midinotecontroller_01_notesdown[129] = { };
    number midinotecontroller_01_note_lastvelocity[128] = { };
    list midinotecontroller_01_muteval;
    signal globaltransport_tempo;
    signal globaltransport_state;
    number stackprotect_count;
    DataRef RNBODefaultMtofLookupTable256;
    DataRef RNBODefaultSinus;
    Index _voiceIndex;
    Int _noteNumber;
    Index isMuted;
    indexlist paramInitIndices;
    indexlist paramInitOrder;
    RNBOSubpatcher_03 poly[16];
    bool _isInitialized = false;
};

static PatcherInterface* creaternbomatic()
{
    return new rnbomatic<EXTERNALENGINE>();
}

#ifndef RNBO_NO_PATCHERFACTORY
extern "C" PatcherFactoryFunctionPtr GetPatcherFactoryFunction()
#else
extern "C" PatcherFactoryFunctionPtr rnbomaticFactoryFunction()
#endif
{
    return creaternbomatic;
}

#ifndef RNBO_NO_PATCHERFACTORY
extern "C" void SetLogger(Logger* logger)
#else
void rnbomaticSetLogger(Logger* logger)
#endif
{
    console = logger;
}

} // end RNBO namespace

