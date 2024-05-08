/**
 * @file llgamecontrol.h
 * @brief GameController detection and management
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llgamecontrol.h"

#include <algorithm>
#include <chrono>
#include <unordered_map>

#include "SDL2/SDL.h"
#include "SDL2/SDL_gamecontroller.h"
#include "SDL2/SDL_joystick.h"

#include "indra_constants.h"
#include "llfile.h"
#include "llgamecontroltranslator.h"

constexpr size_t NUM_AXES = 6;
constexpr size_t NUM_BUTTONS = 32;

// util for dumping SDL_GameController info
std::ostream& operator<<(std::ostream& out, SDL_GameController* c)
{
    if (!c)
    {
        return out << "nullptr";
    }
    out << "{";
    out << " name='" << SDL_GameControllerName(c) << "'";
    out << " type='" << SDL_GameControllerGetType(c) << "'";
    out << " vendor='" << SDL_GameControllerGetVendor(c) << "'";
    out << " product='" << SDL_GameControllerGetProduct(c) << "'";
    out << " version='" << SDL_GameControllerGetProductVersion(c) << "'";
    //CRASH! out << " serial='" << SDL_GameControllerGetSerial(c) << "'";
    out << " }";
    return out;
}

// util for dumping SDL_Joystick info
std::ostream& operator<<(std::ostream& out, SDL_Joystick* j)
{
    if (!j)
    {
        return out << "nullptr";
    }
    out << "{";
    out << " p=0x" << (void*)(j);
    out << " name='" << SDL_JoystickName(j) << "'";
    out << " type='" << SDL_JoystickGetType(j) << "'";
    out << " instance='" << SDL_JoystickInstanceID(j) << "'";
    out << " product='" << SDL_JoystickGetProduct(j) << "'";
    out << " version='" << SDL_JoystickGetProductVersion(j) << "'";
    out << " num_axes=" << SDL_JoystickNumAxes(j);
    out << " num_balls=" << SDL_JoystickNumBalls(j);
    out << " num_hats=" << SDL_JoystickNumHats(j);
    out << " num_buttons=" << SDL_JoystickNumHats(j);
    out << " }";
    return out;
}

std::string LLGameControl::InputChannel::getLocalName() const
{
    // HACK: we hard-code English channel names, but
    // they should be loaded from localized XML config files.

    if ((mType == LLGameControl::InputChannel::TYPE_AXIS) && (mIndex < NUM_AXES))
    {
        return "AXIS_" + std::to_string((U32)mIndex) +
            (mSign < 0 ? "-" : mSign > 0 ? "+" : "");
    }

    if ((mType == LLGameControl::InputChannel::TYPE_BUTTON) && (mIndex < NUM_BUTTONS))
    {
        return "BUTTON_" + std::to_string((U32)mIndex);
    }

    return "NONE";
}

std::string LLGameControl::InputChannel::getRemoteName() const
{
    // HACK: we hard-code English channel names, but
    // they should be loaded from localized XML config files.
    std::string name = " ";
    // GAME_CONTROL_AXIS_LEFTX, GAME_CONTROL_BUTTON_A, etc
    if (mType == LLGameControl::InputChannel::TYPE_AXIS)
    {
        switch (mIndex)
        {
            case 0:
                name = "GAME_CONTROL_AXIS_LEFTX";
                break;
            case 1:
                name = "GAME_CONTROL_AXIS_LEFTY";
                break;
            case 2:
                name = "GAME_CONTROL_AXIS_RIGHTX";
                break;
            case 3:
                name = "GAME_CONTROL_AXIS_RIGHTY";
                break;
            case 4:
                name = "GAME_CONTROL_AXIS_PADDLELEFT";
                break;
            case 5:
                name = "GAME_CONTROL_AXIS_PADDLERIGHT";
                break;
            default:
                break;
        }
    }
    else if (mType == LLGameControl::InputChannel::TYPE_BUTTON)
    {
        switch(mIndex)
        {
            case 0:
                name = "GAME_CONTROL_BUTTON_A";
                break;
            case 1:
                name = "GAME_CONTROL_BUTTON_B";
                break;
            case 2:
                name = "GAME_CONTROL_BUTTON_X";
                break;
            case 3:
                name = "GAME_CONTROL_BUTTON_Y";
                break;
            case 4:
                name = "GAME_CONTROL_BUTTON_BACK";
                break;
            case 5:
                name = "GAME_CONTROL_BUTTON_GUIDE";
                break;
            case 6:
                name = "GAME_CONTROL_BUTTON_START";
                break;
            case 7:
                name = "GAME_CONTROL_BUTTON_LEFTSTICK";
                break;
            case 8:
                name = "GAME_CONTROL_BUTTON_RIGHTSTICK";
                break;
            case 9:
                name = "GAME_CONTROL_BUTTON_LEFTSHOULDER";
                break;
            case 10:
                name = "GAME_CONTROL_BUTTON_RIGHTSHOULDER";
                break;
            case 11:
                name = "GAME_CONTROL_BUTTON_DPAD_UP";
                break;
            case 12:
                name = "GAME_CONTROL_BUTTON_DPAD_DOWN";
                break;
            case 13:
                name = "GAME_CONTROL_BUTTON_DPAD_LEFT";
                break;
            case 14:
                name = "GAME_CONTROL_BUTTON_DPAD_RIGHT";
                break;
            case 15:
                name = "GAME_CONTROL_BUTTON_MISC1";
                break;
            case 16:
                name = "GAME_CONTROL_BUTTON_PADDLE1";
                break;
            case 17:
                name = "GAME_CONTROL_BUTTON_PADDLE2";
                break;
            case 18:
                name = "GAME_CONTROL_BUTTON_PADDLE3";
                break;
            case 19:
                name = "GAME_CONTROL_BUTTON_PADDLE4";
                break;
            case 20:
                name = "GAME_CONTROL_BUTTON_TOUCHPAD";
                break;
            default:
                break;
        }
    }
    return name;
}


// internal class for managing list of controllers and per-controller state
class LLGameControllerManager
{
public:
    using ActionToChannelMap = std::map< std::string, LLGameControl::InputChannel >;
    LLGameControllerManager();

    void initializeMappingsByDefault();

    void addController(SDL_JoystickID id, SDL_GameController* controller);
    void removeController(SDL_JoystickID id);

    void onAxis(SDL_JoystickID id, U8 axis, S16 value);
    void onButton(SDL_JoystickID id, U8 button, bool pressed);

    void clearAllStates();

    void accumulateInternalState();
    void computeFinalState(LLGameControl::State& state);

    LLGameControl::ActionNameType getActionNameType(const std::string& action) const;
    LLGameControl::InputChannel getChannelByAction(const std::string& action) const;
    LLGameControl::InputChannel getFlycamChannelByAction(const std::string& action) const;

    bool updateActionMap(const std::string& name,  LLGameControl::InputChannel channel);
    U32 computeInternalActionFlags();
    void getFlycamInputs(std::vector<F32>& inputs_out);
    void setExternalInput(U32 action_flags, U32 buttons);

    U32 getMappedFlags() const { return mActionTranslator.getMappedFlags(); }

    void clear();

    std::string getAnalogMappings() const;
    std::string getBinaryMappings() const;
    std::string getFlycamMappings() const;

    void setAnalogMappings(const std::string& mappings);
    void setBinaryMappings(const std::string& mappings);
    void setFlycamMappings(const std::string& mappings);

private:
    void updateFlycamMap(const std::string& action, LLGameControl::InputChannel channel);

    std::list<LLGameControl::State> mStates; // one state per device
    using state_it = std::list<LLGameControl::State>::iterator;
    state_it findState(SDL_JoystickID id)
    {
        return std::find_if(mStates.begin(), mStates.end(),
            [id](LLGameControl::State& state)
            {
                return state.getJoystickID() == id;
            });
    }

    LLGameControl::State mExternalState;
    LLGameControlTranslator mActionTranslator;
    std::map<std::string, LLGameControl::ActionNameType> mActions;
    std::vector <std::string> mAnalogActions;
    std::vector <std::string> mBinaryActions;
    std::vector <std::string> mFlycamActions;
    std::vector<LLGameControl::InputChannel> mFlycamChannels;
    std::vector<S32> mAxesAccumulator;
    U32 mButtonAccumulator { 0 };
    U32 mLastActiveFlags { 0 };
    U32 mLastFlycamActionFlags { 0 };

    friend class LLGameControl;
};

// local globals
namespace
{
    LLGameControl* g_gameControl = nullptr;
    LLGameControllerManager g_manager;

    // The GameControlInput message is sent via UDP which is lossy.
    // Since we send the only the list of pressed buttons the receiving
    // side can compute the difference between subsequent states to
    // find button-down/button-up events.
    //
    // To reduce the likelihood of buttons being stuck "pressed" forever
    // on the receiving side (for lost final packet) we resend the last
    // data state. However, to keep the ambient resend bandwidth low we
    // expand the resend period at a geometric rate.
    //
    constexpr U64 MSEC_PER_NSEC = 1e6;
    constexpr U64 FIRST_RESEND_PERIOD = 100 * MSEC_PER_NSEC;
    constexpr U64 RESEND_EXPANSION_RATE = 10;
    LLGameControl::State g_outerState; // from controller devices
    LLGameControl::State g_innerState; // state from gAgent
    LLGameControl::State g_finalState; // sum of inner and outer
    U64 g_lastSend = 0;
    U64 g_nextResendPeriod = FIRST_RESEND_PERIOD;

    bool g_sendToServer = false;
    bool g_controlAgent = false;
    bool g_translateAgentActions = false;
    LLGameControl::AgentControlMode g_agentControlMode = LLGameControl::CONTROL_MODE_AVATAR;

    constexpr U8 MAX_AXIS = NUM_AXES - 1;
    constexpr U8 MAX_BUTTON = NUM_BUTTONS - 1;

    std::function<bool(const std::string&)> s_loadBoolean;
    std::function<void(const std::string&, bool)> s_saveBoolean;
    std::function<std::string(const std::string&)> s_loadString;
    std::function<void(const std::string&, const std::string&)> s_saveString;

    std::string SETTING_SENDTOSERVER("GameControlToServer");
    std::string SETTING_CONTROLAGENT("GameControlToAgent");
    std::string SETTING_TRANSLATEACTIONS("AgentToGameControl");
    std::string SETTING_AGENTCONTROLMODE("AgentControlMode");
    std::string SETTING_ANALOGMAPPINGS("AnalogChannelMappings");
    std::string SETTING_BINARYMAPPINGS("BinaryChannelMappings");
    std::string SETTING_FLYCAMMAPPINGS("FlycamChannelMappings");

    std::string ENUM_AGENTCONTROLMODE_FLYCAM("flycam");
    std::string ENUM_AGENTCONTROLMODE_NONE("none");

    LLGameControl::AgentControlMode convertStringToAgentControlMode(const std::string& mode)
    {
        if (mode == ENUM_AGENTCONTROLMODE_NONE)
            return LLGameControl::CONTROL_MODE_NONE;
        if (mode == ENUM_AGENTCONTROLMODE_FLYCAM)
            return LLGameControl::CONTROL_MODE_FLYCAM;
        // All values except NONE and FLYCAM are treated as default (AVATAR)
        return LLGameControl::CONTROL_MODE_AVATAR;
    }

    std::string convertAgentControlModeToString(LLGameControl::AgentControlMode mode)
    {
        if (mode == LLGameControl::CONTROL_MODE_NONE)
            return ENUM_AGENTCONTROLMODE_NONE;
        if (mode == LLGameControl::CONTROL_MODE_FLYCAM)
            return ENUM_AGENTCONTROLMODE_FLYCAM;
        // All values except NONE and FLYCAM are treated as default (AVATAR)
        return LLStringUtil::null;
    }
}

LLGameControl::~LLGameControl()
{
    terminate();
}

LLGameControl::State::State() : mButtons(0)
{
    mAxes.resize(NUM_AXES, 0);
    mPrevAxes.resize(NUM_AXES, 0);
}

void LLGameControl::State::setDevice(int joystickID, void* controller)
{
    mJoystickID = joystickID;
    mController = controller;
}

void LLGameControl::State::clear()
{
    std::fill(mAxes.begin(), mAxes.end(), 0);

    // DO NOT clear mPrevAxes because those are managed by external logic.
    //std::fill(mPrevAxes.begin(), mPrevAxes.end(), 0);

    mButtons = 0;
}

bool LLGameControl::State::onButton(U8 button, bool pressed)
{
    U32 old_buttons = mButtons;
    if (button <= MAX_BUTTON)
    {
        if (pressed)
        {
            mButtons |= (0x01 << button);
        }
        else
        {
            mButtons &= ~(0x01 << button);
        }
    }
    bool changed = (old_buttons != mButtons);
    return changed;
}

LLGameControllerManager::LLGameControllerManager()
{
    mAxesAccumulator.resize(NUM_AXES, 0);

    mAnalogActions = { "push", "slide", "jump", "turn", "look" };
    mBinaryActions = { "toggle_run", "toggle_fly", "toggle_flycam", "stop" };
    mFlycamActions = { "advance", "pan", "rise", "pitch", "yaw", "zoom" };

    // Collect all known action names with their types in one container
    for (const std::string& name : mAnalogActions)
    {
        mActions[name] = LLGameControl::ACTION_NAME_ANALOG;
        mActions[name + "+"] = LLGameControl::ACTION_NAME_ANALOG_POS;
        mActions[name + "-"] = LLGameControl::ACTION_NAME_ANALOG_NEG;
    }
    for (const std::string& name : mBinaryActions)
    {
        mActions[name] = LLGameControl::ACTION_NAME_BINARY;
    }
    for (const std::string& name : mFlycamActions)
    {
        mActions[name] = LLGameControl::ACTION_NAME_FLYCAM;
    }

    // Here we build an invariant map between the named agent actions
    // and control bit sent to the server.  This map will be used,
    // in combination with the action->InputChannel map below,
    // to maintain an inverse map from control bit masks to GameControl data.
    LLGameControlTranslator::ActionToMaskMap actionMasks =
    {
    // Analog actions (pairs)
        { "push+",  AGENT_CONTROL_AT_POS    | AGENT_CONTROL_FAST_AT   },
        { "push-",  AGENT_CONTROL_AT_NEG    | AGENT_CONTROL_FAST_AT   },
        { "slide+", AGENT_CONTROL_LEFT_POS  | AGENT_CONTROL_FAST_LEFT },
        { "slide-", AGENT_CONTROL_LEFT_NEG  | AGENT_CONTROL_FAST_LEFT },
        { "jump+",  AGENT_CONTROL_UP_POS    | AGENT_CONTROL_FAST_UP   },
        { "jump-",  AGENT_CONTROL_UP_NEG    | AGENT_CONTROL_FAST_UP   },
        { "turn+",  AGENT_CONTROL_YAW_POS   },
        { "turn-",  AGENT_CONTROL_YAW_NEG   },
        { "look+",  AGENT_CONTROL_PITCH_POS },
        { "look-",  AGENT_CONTROL_PITCH_NEG },
    // Button actions
        { "stop",   AGENT_CONTROL_STOP      },
    // These are HACKs. We borrow some AGENT_CONTROL bits for "unrelated" features.
    // Not a problem because these bits are only used internally.
        { "toggle_run",    AGENT_CONTROL_NUDGE_AT_POS }, // HACK
        { "toggle_fly",    AGENT_CONTROL_FLY          }, // HACK
        { "toggle_flycam", AGENT_CONTROL_NUDGE_AT_NEG }, // HACK
    };
    mActionTranslator.setAvailableActionMasks(actionMasks);

    initializeMappingsByDefault();
}

void LLGameControllerManager::initializeMappingsByDefault()
{
    // Here we build a list of pairs between named agent actions and
    // GameControl channels. Note: we only supply the non-signed names
    // (e.g. "push" instead of "push+" and "push-") because mActionTranslator
    // automatially expands action names as necessary.
    using type = LLGameControl::InputChannel::Type;
    std::vector<std::pair<std::string, LLGameControl::InputChannel>> agent_defaults =
    {
    // Analog actions (associated by common name - without '+' or '-')
        { "push",  { type::TYPE_AXIS,   LLGameControl::AXIS_LEFTY,       1 } },
        { "slide", { type::TYPE_AXIS,   LLGameControl::AXIS_LEFTX,       1 } },
        { "jump",  { type::TYPE_AXIS,   LLGameControl::AXIS_TRIGGERLEFT, 1 } },
        { "turn",  { type::TYPE_AXIS,   LLGameControl::AXIS_RIGHTX,      1 } },
        { "look",  { type::TYPE_AXIS,   LLGameControl::AXIS_RIGHTY,      1 } },
    // Button actions (associated by name)
        { "toggle_run",    { type::TYPE_BUTTON, LLGameControl::BUTTON_LEFTSHOULDER }  },
        { "toggle_fly",    { type::TYPE_BUTTON, LLGameControl::BUTTON_DPAD_UP }       },
        { "toggle_flycam", { type::TYPE_BUTTON, LLGameControl::BUTTON_RIGHTSHOULDER } },
        { "stop",          { type::TYPE_BUTTON, LLGameControl::BUTTON_LEFTSTICK }     }
    };
    mActionTranslator.setMappings(agent_defaults);

    // Flycam actions don't need bitwise translation, so we maintain the map
    // of channels here directly rather than using an LLGameControlTranslator.
    mFlycamChannels =
    {
    // Flycam actions (associated just by an order index)
        { type::TYPE_AXIS, LLGameControl::AXIS_LEFTY,        1 }, // advance
        { type::TYPE_AXIS, LLGameControl::AXIS_LEFTX,        1 }, // pan
        { type::TYPE_AXIS, LLGameControl::AXIS_TRIGGERRIGHT, 1 }, // rise
        { type::TYPE_AXIS, LLGameControl::AXIS_RIGHTY,      -1 }, // pitch
        { type::TYPE_AXIS, LLGameControl::AXIS_RIGHTX,       1 }, // yaw
        { type::TYPE_NONE, 0                                   }  // zoom
    };
}

void LLGameControllerManager::addController(SDL_JoystickID id, SDL_GameController* controller)
{
    LL_INFOS("GameController") << "joystick id: " << id << ", controller: " << controller << LL_ENDL;

    llassert(id >= 0);
    llassert(controller);

    if (findState(id) != mStates.end())
    {
        LL_WARNS("GameController") << "device already added" << LL_ENDL;
        return;
    }

    mStates.emplace_back().setDevice(id, controller);
    LL_DEBUGS("SDL2") << "joystick=0x" << std::hex << id << std::dec
        << " controller=" << controller
        << LL_ENDL;
}

void LLGameControllerManager::removeController(SDL_JoystickID id)
{
    LL_INFOS("GameController") << "joystick id: " << id << LL_ENDL;

    mStates.remove_if([id](LLGameControl::State& state)
        {
            return state.getJoystickID() == id;
        });
}

void LLGameControllerManager::onAxis(SDL_JoystickID id, U8 axis, S16 value)
{
    if (axis > MAX_AXIS)
    {
        return;
    }

    state_it it = findState(id);
    if (it != mStates.end())
    {
        // Note: the RAW analog joysticks provide NEGATIVE X,Y values for LEFT,FORWARD
        // whereas those directions are actually POSITIVE in SL's local right-handed
        // reference frame.  Therefore we implicitly negate those axes here where
        // they are extracted from SDL, before being used anywhere.
        if (axis < SDL_CONTROLLER_AXIS_TRIGGERLEFT)
        {
            // Note: S16 value is in range [-32768, 32767] which means
            // the negative range has an extra possible value.  We need
            // to add (or subtract) one during negation.
            if (value < 0)
            {
                value = - (value + 1);
            }
            else if (value > 0)
            {
                value = (-value) - 1;
            }
        }

        LL_DEBUGS("SDL2") << "joystick=0x" << std::hex << id << std::dec
            << " axis=" << (S32)(axis)
            << " value=" << (S32)(value) << LL_ENDL;
        it->mAxes[axis] = value;
    }
}

void LLGameControllerManager::onButton(SDL_JoystickID id, U8 button, bool pressed)
{
    state_it it = findState(id);
    if (it != mStates.end())
    {
        if (it->onButton(button, pressed))
        {
            LL_DEBUGS("SDL2") << "joystick=0x" << std::hex << id << std::dec
                << " button i=" << (S32)(button)
                << " pressed=" << pressed << LL_ENDL;
        }
    }
}

void LLGameControllerManager::clearAllStates()
{
    for (auto& state : mStates)
    {
        state.clear();
    }
    mExternalState.clear();
    mLastActiveFlags = 0;
    mLastFlycamActionFlags = 0;
}

void LLGameControllerManager::accumulateInternalState()
{
    // clear the old state
    std::fill(mAxesAccumulator.begin(), mAxesAccumulator.end(), 0);
    mButtonAccumulator = 0;

    // accumulate the controllers
    for (const auto& state : mStates)
    {
        mButtonAccumulator |= state.mButtons;
        for (size_t i = 0; i < NUM_AXES; ++i)
        {
            // Note: we don't bother to clamp the axes yet
            // because at this stage we haven't yet accumulated the "inner" state.
            mAxesAccumulator[i] += (S32)state.mAxes[i];
        }
    }
}

void LLGameControllerManager::computeFinalState(LLGameControl::State& final_state)
{
    // We assume accumulateInternalState() has already been called and we will
    // finish by accumulating "external" state (if enabled)
    U32 old_buttons = final_state.mButtons;
    final_state.mButtons = mButtonAccumulator;
    if (g_translateAgentActions)
    {
        // accumulate from mExternalState
        final_state.mButtons |= mExternalState.mButtons;
    }
    if (old_buttons != final_state.mButtons)
    {
        g_nextResendPeriod = 0; // packet needs to go out ASAP
    }

    // clamp the accumulated axes
    for (size_t i = 0; i < NUM_AXES; ++i)
    {
        S32 axis = mAxesAccumulator[i];
        if (g_translateAgentActions)
        {
            // Note: we accumulate mExternalState onto local 'axis' variable
            // rather than onto mAxisAccumulator[i] because the internal
            // accumulated value is also used to drive the Flycam, and
            // we don't want any external state leaking into that value.
            axis += (S32)mExternalState.mAxes[i];
        }
        axis = (S16)std::min(std::max(axis, -32768), 32767);
        // check for change
        if (final_state.mAxes[i] != axis)
        {
            // When axis changes we explicitly update the corresponding prevAxis
            // prior to storing axis.  The only other place where prevAxis
            // is updated in updateResendPeriod() which is explicitly called after
            // a packet is sent.  The result is: unchanged axes are included in
            // first resend but not later ones.
            final_state.mPrevAxes[i] = final_state.mAxes[i];
            final_state.mAxes[i] = axis;
            g_nextResendPeriod = 0; // packet needs to go out ASAP
        }
    }
}

LLGameControl::ActionNameType LLGameControllerManager::getActionNameType(const std::string& action) const
{
    auto it = mActions.find(action);
    return it == mActions.end() ? LLGameControl::ACTION_NAME_UNKNOWN : it->second;
}

LLGameControl::InputChannel LLGameControllerManager::getChannelByAction(const std::string& action) const
{
    LLGameControl::InputChannel channel;
    auto action_it = mActions.find(action);
    if (action_it != mActions.end())
    {
        if (action_it->second == LLGameControl::ACTION_NAME_FLYCAM)
        {
            channel = getFlycamChannelByAction(action);
        }
        else
        {
            channel = mActionTranslator.getChannelByAction(action);
        }
    }
    return channel;
}

LLGameControl::InputChannel LLGameControllerManager::getFlycamChannelByAction(const std::string& action) const
{
    auto flycam_it = std::find(mFlycamActions.begin(), mFlycamActions.end(), action);
    llassert(flycam_it != mFlycamActions.end());
    std::ptrdiff_t index = std::distance(mFlycamActions.begin(), flycam_it);
    return mFlycamChannels[(std::size_t)index];
}

// Common implementation of getAnalogMappings(), getBinaryMappings() and getFlycamMappings()
static std::string getMappings(const std::vector<std::string>& actions, LLGameControl::InputChannel::Type type,
    std::function<LLGameControl::InputChannel(const std::string& action)> getChannel)
{
    std::string result;
    // Walk through the all known actions of the chosen type
    for (const std::string& action : actions)
    {
        LLGameControl::InputChannel channel = getChannel(action);
        // Only channels of the expected type should be stored
        if (channel.mType == type)
        {
            result += action + ":" + channel.getLocalName() + ",";
        }
    }
    // Remove the last comma if exists
    if (!result.empty())
    {
        result.resize(result.size() - 1);
    }
    return result;
}

std::string LLGameControllerManager::getAnalogMappings() const
{
    return getMappings(mAnalogActions, LLGameControl::InputChannel::TYPE_AXIS,
        [&](const std::string& action) -> LLGameControl::InputChannel
        {
            return mActionTranslator.getChannelByAction(action + "+");
        });
}

std::string LLGameControllerManager::getBinaryMappings() const
{
    return getMappings(mBinaryActions, LLGameControl::InputChannel::TYPE_BUTTON,
        [&](const std::string& action) -> LLGameControl::InputChannel
        {
            return mActionTranslator.getChannelByAction(action);
        });
}

std::string LLGameControllerManager::getFlycamMappings() const
{
    return getMappings(mFlycamActions, LLGameControl::InputChannel::TYPE_AXIS,
        [&](const std::string& action) -> LLGameControl::InputChannel
        {
            return getFlycamChannelByAction(action);
        });
}

// Common implementation of setAnalogMappings(), setBinaryMappings() and setFlycamMappings()
static void setMappings(const std::string& mappings,
    const std::vector<std::string>& actions, LLGameControl::InputChannel::Type type,
    std::function<void(const std::string& action, LLGameControl::InputChannel channel)> updateMap)
{
    std::map<std::string, std::string> pairs;
    LLStringOps::splitString(mappings, ',', [&](const std::string& mapping)
        {
            std::size_t pos = mapping.find(':');
            if (pos > 0 && pos != std::string::npos)
            {
                pairs[mapping.substr(0, pos)] = mapping.substr(pos + 1);
            }
        });

    static const LLGameControl::InputChannel channelNone;

    for (const std::string& action : actions)
    {
        auto it = pairs.find(action);
        if (it != pairs.end())
        {
            LLGameControl::InputChannel channel = LLGameControl::getChannelByName(it->second);
            if (channel.isNone() || channel.mType == type)
            {
                updateMap(action, channel);
                continue;
            }
        }
        updateMap(action, channelNone);
    }
}

void LLGameControllerManager::setAnalogMappings(const std::string& mappings)
{
    setMappings(mappings, mAnalogActions, LLGameControl::InputChannel::TYPE_AXIS,
        [&](const std::string& action, LLGameControl::InputChannel channel)
        {
            mActionTranslator.updateMap(action, channel);
        });
}

void LLGameControllerManager::setBinaryMappings(const std::string& mappings)
{
    setMappings(mappings, mBinaryActions, LLGameControl::InputChannel::TYPE_BUTTON,
        [&](const std::string& action, LLGameControl::InputChannel channel)
        {
            mActionTranslator.updateMap(action, channel);
        });
}

void LLGameControllerManager::setFlycamMappings(const std::string& mappings)
{
    setMappings(mappings, mFlycamActions, LLGameControl::InputChannel::TYPE_AXIS,
        [&](const std::string& action, LLGameControl::InputChannel channel)
        {
            updateFlycamMap(action, channel);
        });
}

bool LLGameControllerManager::updateActionMap(const std::string& action, LLGameControl::InputChannel channel)
{
    auto action_it = mActions.find(action);
    if (action_it == mActions.end())
    {
        LL_WARNS("GameControl") << "unmappable action='" << action << "'" << LL_ENDL;
        return false;
    }

    if (action_it->second == LLGameControl::ACTION_NAME_FLYCAM)
    {
        updateFlycamMap(action, channel);
    }
    else
    {
        mActionTranslator.updateMap(action, channel);
    }
    return true;
}

void LLGameControllerManager::updateFlycamMap(const std::string& action, LLGameControl::InputChannel channel)
{
    auto flycam_it = std::find(mFlycamActions.begin(), mFlycamActions.end(), action);
    llassert(flycam_it != mFlycamActions.end());
    std::ptrdiff_t index = std::distance(mFlycamActions.begin(), flycam_it);
    llassert(index >= 0 && index < mFlycamChannels.size());
    mFlycamChannels[(std::size_t)index] = channel;
}

U32 LLGameControllerManager::computeInternalActionFlags()
{
    // add up device inputs
    accumulateInternalState();
    if (g_controlAgent)
    {
        return mActionTranslator.computeFlagsFromState(mAxesAccumulator, mButtonAccumulator);
    }
    return 0;
}

void LLGameControllerManager::getFlycamInputs(std::vector<F32>& inputs)
{
    // The inputs are packed in the same order as they exist in mFlycamChannels:
    //
    //     advance
    //     pan
    //     rise
    //     pitch
    //     yaw
    //     zoom
    //
    for (const auto& channel: mFlycamChannels)
    {
        S16 axis;
        if (channel.mIndex == LLGameControl::AXIS_TRIGGERLEFT ||
            channel.mIndex == LLGameControl::AXIS_TRIGGERRIGHT)
        {
            // TIED TRIGGER HACK: we assume the two triggers are paired together
            S32 total_axis = mAxesAccumulator[LLGameControl::AXIS_TRIGGERLEFT]
                - mAxesAccumulator[LLGameControl::AXIS_TRIGGERRIGHT];
            if (channel.mIndex == LLGameControl::AXIS_TRIGGERRIGHT)
            {
                // negate previous math when TRIGGERRIGHT is positive channel
                total_axis *= -1;
            }
            axis = S16(std::min(std::max(total_axis, -32768), 32767));
        }
        else
        {
            axis = S16(std::min(std::max(mAxesAccumulator[channel.mIndex], -32768), 32767));
        }
        // value arrives as S16 in range [-32768, 32767]
        // so we scale positive and negative values by slightly different factors
        // to try to map it to [-1, 1]
        F32 input = F32(axis) / ((axis > 0.0f) ? 32767 : 32768) * channel.mSign;
        inputs.push_back(input);
    }
}

void LLGameControllerManager::setExternalInput(U32 action_flags, U32 buttons)
{
    if (g_translateAgentActions)
    {
        // HACK: these are the bits we can safely translate from control flags to GameControl
        // Extracting LLGameControl::InputChannels that are mapped to other bits is a WIP.
        // TODO: translate other bits to GameControl, which might require measure of gAgent
        // state changes (e.g. sitting <--> standing, flying <--> not-flying, etc)
        const U32 BITS_OF_INTEREST =
            AGENT_CONTROL_AT_POS | AGENT_CONTROL_AT_NEG
            | AGENT_CONTROL_LEFT_POS | AGENT_CONTROL_LEFT_NEG
            | AGENT_CONTROL_UP_POS | AGENT_CONTROL_UP_NEG
            | AGENT_CONTROL_YAW_POS | AGENT_CONTROL_YAW_NEG
            | AGENT_CONTROL_PITCH_POS | AGENT_CONTROL_PITCH_NEG
            | AGENT_CONTROL_STOP
            | AGENT_CONTROL_FAST_AT
            | AGENT_CONTROL_FAST_LEFT
            | AGENT_CONTROL_FAST_UP;
        action_flags &= BITS_OF_INTEREST;

        U32 active_flags = action_flags & mActionTranslator.getMappedFlags();
        if (active_flags != mLastActiveFlags)
        {
            mLastActiveFlags = active_flags;
            mExternalState = mActionTranslator.computeStateFromFlags(action_flags);
            mExternalState.mButtons |= buttons;
        }
        else
        {
            mExternalState.mButtons = buttons;
        }
    }
    else
    {
        mExternalState.mButtons = buttons;
    }
}

void LLGameControllerManager::clear()
{
    mStates.clear();
}

U64 get_now_nsec()
{
    std::chrono::time_point<std::chrono::steady_clock> t0;
    return (std::chrono::steady_clock::now() - t0).count();
}

void onJoystickDeviceAdded(const SDL_Event& event)
{
    LL_INFOS("GameController") << "device index: " << event.cdevice.which << LL_ENDL;

    if (SDL_Joystick* joystick = SDL_JoystickOpen(event.cdevice.which))
    {
        LL_INFOS("GameController") << "joystick: " << joystick << LL_ENDL;
    }
    else
    {
        LL_WARNS("GameController") << "Can't open joystick: " << SDL_GetError() << LL_ENDL;
    }
}

void onJoystickDeviceRemoved(const SDL_Event& event)
{
    LL_INFOS("GameController") << "joystick id: " << event.cdevice.which << LL_ENDL;
}

void onControllerDeviceAdded(const SDL_Event& event)
{
    LL_INFOS("GameController") << "device index: " << event.cdevice.which << LL_ENDL;

    SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(event.cdevice.which);
    if (id < 0)
    {
        LL_WARNS("GameController") << "Can't get device instance ID: " << SDL_GetError() << LL_ENDL;
        return;
    }

    SDL_GameController* controller = SDL_GameControllerOpen(event.cdevice.which);
    if (!controller)
    {
        LL_WARNS("GameController") << "Can't open game controller: " << SDL_GetError() << LL_ENDL;
        return;
    }

    g_manager.addController(id, controller);
}

void onControllerDeviceRemoved(const SDL_Event& event)
{
    LL_INFOS("GameController") << "joystick id=" << event.cdevice.which << LL_ENDL;

    SDL_JoystickID id = event.cdevice.which;
    g_manager.removeController(id);
}

void onControllerButton(const SDL_Event& event)
{
    g_manager.onButton(event.cbutton.which, event.cbutton.button, event.cbutton.state == SDL_PRESSED);
}

void onControllerAxis(const SDL_Event& event)
{
    LL_DEBUGS("SDL2") << "joystick=0x" << std::hex << event.caxis.which << std::dec
        << " axis=" << (S32)(event.caxis.axis)
        << " value=" << (S32)(event.caxis.value) << LL_ENDL;
    g_manager.onAxis(event.caxis.which, event.caxis.axis, event.caxis.value);
}

// static
bool LLGameControl::isInitialized()
{
    return g_gameControl != nullptr;
}

void sdl_logger(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    LL_DEBUGS("SDL2") << "log='" << message << "'" << LL_ENDL;
}

// static
void LLGameControl::init(const std::string& gamecontrollerdb_path,
    std::function<bool(const std::string&)> loadBoolean,
    std::function<void(const std::string&, bool)> saveBoolean,
    std::function<std::string(const std::string&)> loadString,
    std::function<void(const std::string&, const std::string&)> saveString)
{
    if (g_gameControl)
        return;

    llassert(loadBoolean);
    llassert(saveBoolean);
    llassert(loadString);
    llassert(saveString);

    int result = SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    if (result < 0)
    {
        // This error is critical, we stop working with SDL and return
        LL_WARNS("GameController") << "Error initializing the subsystems : " << SDL_GetError() << LL_ENDL;
        return;
    }

    SDL_LogSetOutputFunction(&sdl_logger, nullptr);

    // The inability to read this file is not critical, we can continue working
    if (!LLFile::isfile(gamecontrollerdb_path.c_str()))
    {
        LL_WARNS("GameController") << "Device mapping db file not found: " << gamecontrollerdb_path << LL_ENDL;
    }
    else
    {
        int count = SDL_GameControllerAddMappingsFromFile(gamecontrollerdb_path.c_str());
        if (count < 0)
        {
            LL_WARNS("GameController") << "Error adding mappings from " << gamecontrollerdb_path << " : " << SDL_GetError() << LL_ENDL;
        }
        else
        {
            LL_INFOS("GameController") << "Total " << count << " mappings added from " << gamecontrollerdb_path << LL_ENDL;
        }
    }

    g_gameControl = LLGameControl::getInstance();

    s_loadBoolean = loadBoolean;
    s_saveBoolean = saveBoolean;
    s_loadString = loadString;
    s_saveString = saveString;

    loadFromSettings();
}

// static
void LLGameControl::terminate()
{
    g_manager.clear();
    SDL_Quit();
}

//static
// returns 'true' if GameControlInput message needs to go out,
// which will be the case for new data or resend. Call this right
// before deciding to put a GameControlInput packet on the wire
// or not.
bool LLGameControl::computeFinalStateAndCheckForChanges()
{
    // Note: LLGameControllerManager::computeFinalState() modifies g_nextResendPeriod as a side-effect
    g_manager.computeFinalState(g_finalState);

    // should send input when:
    //     sending is enabled and
    //     g_lastSend has "expired"
    //         either because g_nextResendPeriod has been zeroed
    //         or the last send really has expired.
    return g_sendToServer && (g_lastSend + g_nextResendPeriod < get_now_nsec());
}

// static
void LLGameControl::clearAllStates()
{
    g_manager.clearAllStates();
}

// static
void LLGameControl::processEvents(bool app_has_focus)
{
    SDL_Event event;
    if (!app_has_focus)
    {
        // when SL window lacks focus: pump SDL events but ignore them
        while (g_gameControl && SDL_PollEvent(&event))
        {
            // do nothing: SDL_PollEvent() is the operator
        }
        g_manager.clearAllStates();
        return;
    }

    while (g_gameControl && SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_JOYDEVICEADDED:
                onJoystickDeviceAdded(event);
                break;
            case SDL_JOYDEVICEREMOVED:
                onJoystickDeviceRemoved(event);
                break;
            case SDL_CONTROLLERDEVICEADDED:
                onControllerDeviceAdded(event);
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                onControllerDeviceRemoved(event);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                /* FALLTHROUGH */
            case SDL_CONTROLLERBUTTONUP:
                onControllerButton(event);
                break;
            case SDL_CONTROLLERAXISMOTION:
                onControllerAxis(event);
                break;
            default:
                break;
        }
    }
}

// static
const LLGameControl::State& LLGameControl::getState()
{
    return g_finalState;
}

// static
LLGameControl::InputChannel LLGameControl::getActiveInputChannel()
{
    InputChannel input;

    State state = g_finalState;
    if (state.mButtons > 0)
    {
        // check buttons
        input.mType = LLGameControl::InputChannel::TYPE_BUTTON;
        for (U8 i = 0; i < 32; ++i)
        {
            if ((0x1 << i) & state.mButtons)
            {
                input.mIndex = i;
                break;
            }
        }
    }
    else
    {
        // scan axes
        S16 threshold = std::numeric_limits<S16>::max() / 2;
        for (U8 i = 0; i < 6; ++i)
        {
            if (abs(state.mAxes[i]) > threshold)
            {
                input.mType = LLGameControl::InputChannel::TYPE_AXIS;
                // input.mIndex ultimately translates to a LLGameControl::KeyboardAxis
                // which distinguishes between negative and positive directions
                // so we must translate to axis index "i" according to the sign
                // of the axis value.
                input.mIndex = i;
                input.mSign = state.mAxes[i] > 0 ? 1 : -1;
                break;
            }
        }
    }

    return input;
}

// static
void LLGameControl::getFlycamInputs(std::vector<F32>& inputs_out)
{
    return g_manager.getFlycamInputs(inputs_out);
}

// static
void LLGameControl::setSendToServer(bool enable)
{
    g_sendToServer = enable;
    s_saveBoolean(SETTING_SENDTOSERVER, g_sendToServer);
}

// static
void LLGameControl::setControlAgent(bool enable)
{
    g_controlAgent = enable;
    s_saveBoolean(SETTING_CONTROLAGENT, g_controlAgent);
}

// static
void LLGameControl::setTranslateAgentActions(bool enable)
{
    g_translateAgentActions = enable;
    s_saveBoolean(SETTING_TRANSLATEACTIONS, g_translateAgentActions);
}

// static
void LLGameControl::setAgentControlMode(LLGameControl::AgentControlMode mode)
{
    g_agentControlMode = mode;
    s_saveString(SETTING_AGENTCONTROLMODE, convertAgentControlModeToString(mode));
}

// static
bool LLGameControl::getSendToServer()
{
    return g_sendToServer;
}

// static
bool LLGameControl::getControlAgent()
{
    return g_controlAgent;
}

// static
bool LLGameControl::getTranslateAgentActions()
{
    return g_translateAgentActions;
}

// static
LLGameControl::AgentControlMode LLGameControl::getAgentControlMode()
{
    return g_agentControlMode;
}

// static
LLGameControl::ActionNameType LLGameControl::getActionNameType(const std::string& action)
{
    return g_manager.getActionNameType(action);
}

// static
bool LLGameControl::willControlAvatar()
{
    return g_controlAgent && g_agentControlMode == CONTROL_MODE_AVATAR;
}

// static
//
// Given a name like "AXIS_1-" or "BUTTON_5" returns the corresponding InputChannel
// If the axis name lacks the +/- postfix it assumes '+' postfix.
LLGameControl::InputChannel LLGameControl::getChannelByName(const std::string& name)
{
    LLGameControl::InputChannel channel;

    // 'name' has two acceptable formats: AXIS_<index>[sign] or BUTTON_<index>
    if (!strncmp(name.c_str(), "AXIS_", 5))
    {
        channel.mType = LLGameControl::InputChannel::Type::TYPE_AXIS;
        // Decimal postfix is only one character
        channel.mIndex = atoi(name.substr(5, 1).c_str());
        // AXIS_n can have an optional +/- at index 6
        // Assume positive axis when sign not provided
        channel.mSign = name.back() == '-' ? -1 : 1;
    }
    else if (!strncmp(name.c_str(), "BUTTON_", 7))
    {
        channel.mType = LLGameControl::InputChannel::Type::TYPE_BUTTON;
        // Decimal postfix is only one or two characters
        channel.mIndex = atoi(name.substr(7, 2).c_str());
    }

    return channel;
}

// static
// Given an action_name like "push+", or "strafe-", returns the InputChannel
// mapped to it if found, else channel.isNone() will be true.
LLGameControl::InputChannel LLGameControl::getChannelByAction(const std::string& action)
{
    return g_manager.getChannelByAction(action);
}

// static
bool LLGameControl::updateActionMap(const std::string& action, LLGameControl::InputChannel channel)
{
    return g_manager.updateActionMap(action, channel);
}

// static
U32 LLGameControl::computeInternalActionFlags()
{
    return g_manager.computeInternalActionFlags();
}

// static
void LLGameControl::setExternalInput(U32 action_flags, U32 buttons)
{
    g_manager.setExternalInput(action_flags, buttons);
}

//static
void LLGameControl::updateResendPeriod()
{
    // we expect this method to be called right after data is sent
    g_lastSend = get_now_nsec();
    if (g_nextResendPeriod == 0)
    {
        g_nextResendPeriod = FIRST_RESEND_PERIOD;
    }
    else
    {
        // Reset mPrevAxes only on second resend or higher
        // because when the joysticks are being used we expect a steady stream
        // of recorrection data rather than sparse changes.
        //
        // (The above assumption is not necessarily true for "Actions" input
        // (e.g. keyboard events).  TODO: figure out what to do about this.)
        //
        // In other words: we want to include changed axes in the first resend
        // so we only overwrite g_finalState.mPrevAxes on higher resends.
        g_finalState.mPrevAxes = g_finalState.mAxes;
        g_nextResendPeriod *= RESEND_EXPANSION_RATE;
    }
}

// static
std::string LLGameControl::stringifyAnalogMappings(getChannel_t getChannel)
{
    return getMappings(g_manager.mAnalogActions, InputChannel::TYPE_AXIS, getChannel);
}

// static
std::string LLGameControl::stringifyBinaryMappings(getChannel_t getChannel)
{
    return getMappings(g_manager.mBinaryActions, InputChannel::TYPE_BUTTON, getChannel);
}

// static
std::string LLGameControl::stringifyFlycamMappings(getChannel_t getChannel)
{
    return getMappings(g_manager.mFlycamActions, InputChannel::TYPE_AXIS, getChannel);
}

// static
void LLGameControl::initByDefault()
{
    g_sendToServer = false;
    g_controlAgent = false;
    g_translateAgentActions = false;
    g_agentControlMode = CONTROL_MODE_AVATAR;
    g_manager.initializeMappingsByDefault();
}

// static
void LLGameControl::loadFromSettings()
{
    // In case of absence of the required setting the default value is assigned
    g_sendToServer = s_loadBoolean(SETTING_SENDTOSERVER);
    g_controlAgent = s_loadBoolean(SETTING_CONTROLAGENT);
    g_translateAgentActions = s_loadBoolean(SETTING_TRANSLATEACTIONS);
    g_agentControlMode = convertStringToAgentControlMode(s_loadString(SETTING_AGENTCONTROLMODE));

    // Load action-to-channel mappings
    std::string analogMappings = s_loadString(SETTING_ANALOGMAPPINGS);
    std::string binaryMappings = s_loadString(SETTING_BINARYMAPPINGS);
    std::string flycamMappings = s_loadString(SETTING_FLYCAMMAPPINGS);
    // In case of absence of all required settings the default values are assigned
    if (analogMappings.empty() && binaryMappings.empty() && flycamMappings.empty())
    {
        g_manager.initializeMappingsByDefault();
    }
    else
    {
        g_manager.setAnalogMappings(analogMappings);
        g_manager.setBinaryMappings(binaryMappings);
        g_manager.setFlycamMappings(flycamMappings);
        if (!g_manager.getMappedFlags()) // No action is mapped?
        {
            g_manager.initializeMappingsByDefault();
        }
    }
}

// static
void LLGameControl::saveToSettings()
{
    s_saveBoolean(SETTING_SENDTOSERVER, g_sendToServer);
    s_saveBoolean(SETTING_CONTROLAGENT, g_controlAgent);
    s_saveBoolean(SETTING_TRANSLATEACTIONS, g_translateAgentActions);
    s_saveString(SETTING_AGENTCONTROLMODE, convertAgentControlModeToString(g_agentControlMode));
    s_saveString(SETTING_ANALOGMAPPINGS, g_manager.getAnalogMappings());
    s_saveString(SETTING_BINARYMAPPINGS, g_manager.getBinaryMappings());
    s_saveString(SETTING_FLYCAMMAPPINGS, g_manager.getFlycamMappings());
}
