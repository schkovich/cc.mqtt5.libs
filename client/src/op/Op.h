//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ExtConfig.h"
#include "ObjListType.h"
#include "PropsHandler.h"
#include "ProtocolDefs.h"

#include "cc_mqtt5_client/common.h"

namespace cc_mqtt5_client
{

class ClientImpl;

namespace op
{

class Op : public ProtMsgHandler
{
public:
    enum Type
    {
        Type_Connect,
        Type_KeepAlive,
        Type_Disconnect,
        Type_Subscribe,
        Type_Unsubscribe,
        Type_NumOfValues // Must be last
    };

    virtual ~Op() noexcept = default;

    Type type() const
    {
        return typeImpl();
    }

    void terminateOp(CC_Mqtt5AsyncOpStatus status)
    {
        terminateOpImpl(status);
    }

    unsigned getResponseTimeout() const
    {
        return m_responseTimeoutMs;
    }

    void setResponseTimeout(unsigned ms)
    {
        m_responseTimeoutMs = ms;
    }    

protected:
    using UserPropsList = ObjListType<CC_Mqtt5UserProp, Config::UserPropsLimit, Config::HasUserProps>;

    explicit Op(ClientImpl& client);

    virtual Type typeImpl() const = 0;
    virtual void terminateOpImpl(CC_Mqtt5AsyncOpStatus status);

    void sendMessage(const ProtMessage& msg);
    void opComplete();
    void doApiGuard();
    unsigned allocPacketId();

    ClientImpl& client()
    {
        return m_client;
    }

    void sendDisconnectWithReason(DisconnectMsg::Field_reasonCode::Field::ValueType reason);

    static void fillUserProps(const PropsHandler& propsHandler, UserPropsList& userProps);

    template <typename TField>
    static bool canAddProp(const TField& field)
    {
        return field.value().size() < field.value().max_size();
    }

    template <typename TField>
    static decltype(auto) addProp(TField& field)
    {
        auto& vec = field.value();
        vec.resize(vec.size() + 1U);
        return vec.back();    
    }

    template <typename TField>
    static CC_Mqtt5ErrorCode addUserPropToList(TField& field, const CC_Mqtt5UserProp& prop)
    {
        if constexpr (ExtConfig::HasUserProps) {
            if (prop.m_key == nullptr) {
                return CC_Mqtt5ErrorCode_BadParam;
            }

            if (!canAddProp(field)) {
                return CC_Mqtt5ErrorCode_OutOfMemory;
            }

            auto& propVar = addProp(field);
            auto& propBundle = propVar.initField_userProperty();
            auto& valueField = propBundle.field_value();
            valueField.field_first().value() = prop.m_key;

            if (prop.m_value != nullptr) {
                valueField.field_second().value() = prop.m_value;
            }

            return CC_Mqtt5ErrorCode_Success;
        }
        else {
            return CC_Mqtt5ErrorCode_NotSupported;
        }        
    }    

private:
    ClientImpl& m_client;    
    unsigned m_responseTimeoutMs = 0U;
};

} // namespace op

} // namespace cc_mqtt5_client
