// Copyright (c) 2022 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef IOX_EXAMPLES_AUTOMOTIVE_SOA_FIELD_PUBLISHER_INL
#define IOX_EXAMPLES_AUTOMOTIVE_SOA_FIELD_PUBLISHER_INL

#include "owl/kom/field_publisher.hpp"

namespace owl
{
namespace kom
{
template <typename T>
inline FieldPublisher<T>::FieldPublisher(const core::String& service,
                                         const core::String& instance,
                                         const core::String& event,
                                         FieldType& field) noexcept
    : m_publisher({service, instance, event}, {HISTORY_CAPACITY})
    , m_server({service, instance, event})
    , m_latestValue(field)
{
    // publisher is automatically offered
    Update(field);

    m_listener
        .attachEvent(m_server,
                     iox::popo::ServerEvent::REQUEST_RECEIVED,
                     iox::popo::createNotificationCallback(onRequestReceived, *this))
        .or_else([](auto) {
            std::cerr << "unable to attach server" << std::endl;
            std::exit(EXIT_FAILURE);
        });
}

template <typename T>
inline FieldPublisher<T>::~FieldPublisher() noexcept
{
    m_listener.detachEvent(m_server, iox::popo::ServerEvent::REQUEST_RECEIVED);
    m_publisher.stopOffer();
}

template <typename T>
inline void FieldPublisher<T>::Update(const FieldType& userField) noexcept
{
    auto maybeField = m_publisher.loan();

    if (maybeField.has_error())
    {
        std::cerr << "Error occured during allocation, couldn't send sample!" << std::endl;
        return;
    }

    auto& field = maybeField.value();
    *(field.get()) = userField;
    m_latestValue = userField;
    field.publish();
}

template <typename T>
inline void FieldPublisher<T>::RegisterGetHandler(iox::cxx::function<void()>) noexcept
{
    std::cerr << "'RegisterGetHandler' not implemented." << std::endl;
}

template <typename T>
inline void FieldPublisher<T>::RegisterSetHandler(iox::cxx::function<void()>) noexcept
{
    std::cerr << "'RegisterSetHandler' not implemented." << std::endl;
}

template <typename T>
inline void FieldPublisher<T>::onRequestReceived(iox::popo::Server<iox::cxx::optional<FieldType>, FieldType>* server,
                                                 FieldPublisher<FieldType>* self) noexcept
{
    while (server->take().and_then([&](const auto& request) {
        server->loan(request)
            .and_then([&](auto& response) {
                if (request->has_value())
                {
                    self->m_latestValue = request->value();
                }
                *response = self->m_latestValue;
                response.send().or_else(
                    [&](auto& error) { std::cerr << "Could not send Response! Error: " << error << std::endl; });
            })
            .or_else([](auto& error) { std::cerr << "Could not allocate Response! Error: " << error << std::endl; });
    }))
    {
    }
}
} // namespace kom
} // namespace owl

#endif // IOX_EXAMPLES_AUTOMOTIVE_SOA_FIELD_PUBLISHER_INL