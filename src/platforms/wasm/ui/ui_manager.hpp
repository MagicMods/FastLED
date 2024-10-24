#pragma once

#include "fixed_map.h"
#include "third_party/arduinojson/json.h"
#include "ui_manager.h"
#include <emscripten.h>
#include <sstream>
#include <vector>

FASTLED_NAMESPACE_BEGIN

inline bool jsUiManager::WeakPtrCompare::operator()(
    const WeakPtr<jsUiInternal> &lhs,
    const WeakPtr<jsUiInternal> &rhs) const {
    auto l = lhs.lock();
    auto r = rhs.lock();
    if (!l && !r)
        return false;
    if (!l)
        return true;
    if (!r)
        return false;
    return l->id() < r->id();
}

inline void jsUiManager::addComponent(WeakPtr<jsUiInternal> component) {
    std::lock_guard<std::mutex> lock(instance().mMutex);
    instance().mComponents.insert(component);
    instance().mItemsAdded = true;
}

inline void
jsUiManager::removeComponent(WeakPtr<jsUiInternal> component) {
    std::lock_guard<std::mutex> lock(instance().mMutex);
    instance().mComponents.erase(component);
}

inline jsUiManager &jsUiManager::instance() {
    return Singleton<jsUiManager>::instance();
}

inline std::vector<jsUiInternalPtr> jsUiManager::getComponents() {
    std::vector<jsUiInternalPtr> components;
    {
        std::lock_guard<std::mutex> lock(mMutex);

        components.reserve(mComponents.size());
        for (auto it = mComponents.begin(); it != mComponents.end();) {
            if (auto component = it->lock()) {
                components.push_back(component);
                ++it;
            } else {
                it = mComponents.erase(it);
            }
        }
    }
    return components;
}

inline void jsUiManager::updateUiComponents(const std::string &jsonStr) {
    const char *cstr = jsonStr.c_str();
    ArduinoJson::JsonDocument doc;
    ArduinoJson::DeserializationError error =
        ArduinoJson::deserializeJson(doc, cstr);
    if (error) {
        printf("Error: Failed to parse JSON string: %s\n", error.c_str());
        return;
    }
    auto &self = instance();
    self.mPendingJsonUpdate = doc;
    self.mHasPendingUpdate = true;
}

inline void
jsUiManager::executeUiUpdates(const ArduinoJson::JsonDocument &doc) {
    auto& self = instance();
    for (ArduinoJson::JsonPairConst kv :
         doc.as<ArduinoJson::JsonObjectConst>()) {
        int id = atoi(kv.key().c_str());
        // double loop to avoid copying the value
        for (auto it = self.mComponents.begin(); it != self.mComponents.end();) {
            if (auto component = it->lock()) {
                ++it;
                if (component->id() == id) {
                    component->update(kv.value());
                }
            } else {
                it = self.mComponents.erase(it);
            }
        }
    }
}

inline void jsUiManager::toJson(ArduinoJson::JsonArray &json) {
    std::vector<jsUiInternalPtr> components =
        instance().getComponents();
    for (const auto &component : components) {
        ArduinoJson::JsonObject componentJson =
            json.add<ArduinoJson::JsonObject>();
        component->toJson(componentJson);
        if (componentJson.size() == 0) {
            printf("Warning: Empty JSON from component\n");
            json.remove(json.size() - 1);
        }
    }
}

EMSCRIPTEN_BINDINGS(js_interface) {
    emscripten::function("_jsUiManager_updateUiComponents",
                         &jsUiManager::updateUiComponents);
}

FASTLED_NAMESPACE_END
