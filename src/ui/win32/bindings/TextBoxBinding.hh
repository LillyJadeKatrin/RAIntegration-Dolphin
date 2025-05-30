#ifndef RA_UI_WIN32_TEXTBOXBINDING_H
#define RA_UI_WIN32_TEXTBOXBINDING_H
#pragma once

#include "ControlBinding.hh"

#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class TextBoxBinding : public ControlBinding
{
public:
    explicit TextBoxBinding(ViewModelBase& vmViewModel) noexcept : ControlBinding(vmViewModel) {}

    void SetHWND(DialogBase& pDialog, HWND hControl) override
    {
        ControlBinding::SetHWND(pDialog, hControl);

        UpdateBoundText();
        UpdateReadOnly();

        if (!m_mKeyHandlers.empty())
            SubclassWndProc();
    }

    enum class UpdateMode
    {
        None = 0,  // one way from source
        LostFocus, // update source when control loses focus
        KeyPress,  // update source after each key press
        Typing,    // update source 500ms after a key press
    };

    void BindText(const StringModelProperty& pSourceProperty, UpdateMode nUpdateMode = UpdateMode::LostFocus)
    {
        m_pTextBoundProperty = &pSourceProperty;
        m_pTextUpdateMode = nUpdateMode;

        UpdateBoundText();
    }

    void BindReadOnly(const BoolModelProperty& pReadOnlyProperty)
    {
        m_pReadOnlyProperty = &pReadOnlyProperty;

        UpdateReadOnly();
    }

    void OnLostFocus() override
    {
        if (m_pTextUpdateMode == UpdateMode::LostFocus)
            UpdateSource();
    }

    void OnValueChanged() override
    {
        if (m_pTextUpdateMode == UpdateMode::KeyPress)
            UpdateSource();
        else if (m_pTextUpdateMode == UpdateMode::Typing)
            UpdateSourceDelayed();
    }

    void BindKey(unsigned int nKey, std::function<bool()> pHandler)
    {
        if (m_mKeyHandlers.empty() && m_hWnd)
            SubclassWndProc();

        m_mKeyHandlers.insert_or_assign(nKey, pHandler);
    }

    void UpdateSource()
    {
        std::wstring sBuffer;
        GetText(sBuffer);

        UpdateSourceFromText(sBuffer);
    }

protected:
    void GetText(std::wstring& sBuffer)
    {
        const int nLength = GetWindowTextLengthW(m_hWnd);
        sBuffer.resize(gsl::narrow_cast<size_t>(nLength) + 1);
        GetWindowTextW(m_hWnd, sBuffer.data(), gsl::narrow_cast<int>(sBuffer.capacity()));
        sBuffer.resize(nLength);
    }

    void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) override
    {
        if (m_pTextBoundProperty && *m_pTextBoundProperty == args.Property)
            UpdateBoundText();
    }

    void OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) override
    {
        if (m_pReadOnlyProperty && *m_pReadOnlyProperty == args.Property)
        {
            InvokeOnUIThread([this]() {
                UpdateReadOnly();
            });
        }
    }

    INT_PTR CALLBACK WndProc(HWND hControl, UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        switch (uMsg)
        {
            case WM_KEYDOWN:
            {
                const auto iter = m_mKeyHandlers.find(static_cast<unsigned int>(wParam));
                if (iter != m_mKeyHandlers.end())
                {
                    if (iter->second())
                        return 0;
                }
                break;
            }
        }

        return ControlBinding::WndProc(hControl, uMsg, wParam, lParam);
    }

    void UpdateReadOnly()
    {
        const bool bReadOnly = (m_pReadOnlyProperty) ? GetValue(*m_pReadOnlyProperty) : false;
        if (m_hWnd)
            SendMessage(m_hWnd, EM_SETREADONLY, bReadOnly ? TRUE : FALSE, 0);
    }

    virtual void UpdateBoundText()
    {
        if (m_hWnd && m_pTextBoundProperty)
            UpdateTextFromSource(GetValue(*m_pTextBoundProperty));
    }

    virtual void UpdateTextFromSource(const std::wstring& sText) noexcept(false)
    {
        InvokeOnUIThread([this, sTextCopy = sText]() noexcept {
            SetWindowTextW(m_hWnd, sTextCopy.c_str());
        });
    }

    virtual void UpdateSourceFromText(const std::wstring& sText)
    {
        SetValue(*m_pTextBoundProperty, sText);
    }

    void UpdateSourceDelayed()
    {
        static int nVersion = 0;
        const int nCapturedVersion = ++nVersion;

        ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().ScheduleAsync(
            std::chrono::milliseconds(300), [this, nCapturedVersion]()
            {
                if (nCapturedVersion == nVersion)
                    UpdateSource();
            });
    }

    UpdateMode m_pTextUpdateMode = UpdateMode::None;

private:
    const StringModelProperty* m_pTextBoundProperty = nullptr;
    const BoolModelProperty* m_pReadOnlyProperty = nullptr;
    std::map<unsigned int, std::function<bool()>> m_mKeyHandlers;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_TEXTBOXBINDING_H
