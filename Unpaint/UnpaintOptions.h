#pragma once
#include "Storage/SettingManager.h"
#include "DeviceInformation.h"

namespace winrt::Unpaint
{
  class UnpaintOptions
  {
  public:
    UnpaintOptions();

    bool HasShownShowcaseView() const;
    void HasShownShowcaseView(bool value);

    bool HasShownWelcomeView() const;
    void HasShownWelcomeView(bool value);

    bool IsSafeModeEnabled() const;
    void IsSafeModeEnabled(bool value);

    bool IsSafetyCheckerEnabled() const;
    void IsSafetyCheckerEnabled(bool value);

    bool IsDenoiserPinned() const;
    void IsDenoiserPinned(bool value);

    uint32_t AdapterIndex() const;
    void AdapterIndex(uint32_t value);

    std::string ModelId() const;
    void ModelId(const std::string& value) const;

  private:
    static const char* _hasShownShowcaseViewKey;
    static const char* _hasShownWelcomeViewKey;
    static const char* _isSafeModeEnabledKey;
    static const char* _isSafetyCheckerEnabledKey;
    static const char* _isDenoiserPinnedKey;
    static const char* _adapterIndexKey;
    static const char* _modelIdKey;

    std::shared_ptr<Axodox::Storage::SettingManager> _settingManager;
    std::shared_ptr<DeviceInformation> _deviceInformation;
  };
}