#pragma once

#include "engine_events.h"
#include "singleton.h"
#include <mutex>
#include <set>
#include <string>
#include <memory>
#include <map>


#include "fixed_map.h"
#include "ptr.h"

#include "platforms/wasm/js.h"
#include "third_party/arduinojson/json.h"

FASTLED_NAMESPACE_BEGIN

class jsUiInternal;

class jsUiManager : EngineEvents::Listener {
  public:
    static void addComponent(WeakPtr<jsUiInternal> component);
    static void removeComponent(WeakPtr<jsUiInternal> component);

    // Called from the JS engine.
    static void updateUiComponents(const std::string& jsonStr);

  private:
   static void executeUiUpdates(const ArduinoJson::JsonDocument& doc);
    struct WeakPtrCompare {
        bool operator()(const WeakPtr<jsUiInternal>& lhs, const WeakPtr<jsUiInternal>& rhs) const;
    };
    typedef std::set<WeakPtr<jsUiInternal>, WeakPtrCompare> jsUIPtrSet;
    friend class Singleton<jsUiManager>;
    jsUiManager() {
        EngineEvents::addListener(this);
    }
    ~jsUiManager() {
        EngineEvents::removeListener(this);
    }

    void onPlatformPreLoop() override {
        if (!mHasPendingUpdate) {
            return;
        }
        jsUiManager::executeUiUpdates(mPendingJsonUpdate);
        mPendingJsonUpdate.clear();
        mHasPendingUpdate = false;
    }

    void onEndShowLeds() override {
        if (mItemsAdded) {
            // std::string jsonStr = toJsonStr();
            ArduinoJson::JsonDocument doc;
            ArduinoJson::JsonArray jarray = doc.to<ArduinoJson::JsonArray>();
            toJson(jarray);
            // conver to c_str()
            char buff[1024*16] = {0};
            ArduinoJson::serializeJson(doc, buff, sizeof(buff));
            updateJs(buff);
            mItemsAdded = false;
        }
    }

    std::vector<jsUiInternalPtr> getComponents();
    void toJson(ArduinoJson::JsonArray& json);

    jsUIPtrSet mComponents;
    std::mutex mMutex;

    static jsUiManager &instance();
    bool mItemsAdded = false;
    ArduinoJson::JsonDocument mPendingJsonUpdate;
    bool mHasPendingUpdate = false;
};

FASTLED_NAMESPACE_END
