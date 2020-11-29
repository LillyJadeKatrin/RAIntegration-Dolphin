#ifndef RA_UI_ASSETLIST_VIEW_MODEL_H
#define RA_UI_ASSETLIST_VIEW_MODEL_H
#pragma once

#include "ui\WindowViewModelBase.hh"
#include "ui\ViewModelCollection.hh"
#include "ui\viewmodels\LookupItemViewModel.hh"

#include "data\DataModelCollection.hh"
#include "data\Types.hh"
#include "data\models\AchievementModel.hh"
#include "data\models\AssetModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class AssetListViewModel : public WindowViewModelBase,
    protected ra::data::DataModelCollectionBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 AssetListViewModel() noexcept;
    ~AssetListViewModel();

    AssetListViewModel(const AssetListViewModel&) noexcept = delete;
    AssetListViewModel& operator=(const AssetListViewModel&) noexcept = delete;
    AssetListViewModel(AssetListViewModel&&) noexcept = delete;
    AssetListViewModel& operator=(AssetListViewModel&&) noexcept = delete;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the game ID.
    /// </summary>
    static const IntModelProperty GameIdProperty;

    /// <summary>
    /// Gets the game ID.
    /// </summary>
    unsigned GetGameId() const { return ra::to_unsigned(GetValue(GameIdProperty)); }

    /// <summary>
    /// Sets the game ID.
    /// </summary>
    void SetGameId(unsigned nValue) { SetValue(GameIdProperty, ra::to_signed(nValue)); }

    // <summary>
    /// The <see cref="ModelProperty" /> for the core achievement count.
    /// </summary>
    static const IntModelProperty AchievementCountProperty;

    /// <summary>
    /// Gets the total number of core achievements.
    /// </summary>
    int GetAchievementCount() const { return GetValue(AchievementCountProperty); }

    // <summary>
    /// The <see cref="ModelProperty" /> for the total core achievement points.
    /// </summary>
    static const IntModelProperty TotalPointsProperty;

    /// <summary>
    /// Gets the total number of points associated to core achievements.
    /// </summary>
    int GetTotalPoints() const { return GetValue(TotalPointsProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the whether processing is active.
    /// </summary>
    static const BoolModelProperty IsProcessingActiveProperty;

    /// <summary>
    /// Gets whether processing is active.
    /// </summary>
    bool IsProcessingActive() const { return GetValue(IsProcessingActiveProperty); }

    /// <summary>
    /// Sets whether processing is active.
    /// </summary>
    void SetProcessingActive(bool bValue) { SetValue(IsProcessingActiveProperty, bValue); }

    /// <summary>
    /// Gets the list of asset states.
    /// </summary>
    const LookupItemViewModelCollection& States() const noexcept
    {
        return m_vStates;
    }

    /// <summary>
    /// Gets the list of asset categories.
    /// </summary>
    const LookupItemViewModelCollection& Categories() const noexcept
    {
        return m_vCategories;
    }

    /// <summary>
    /// Gets the list of asset modification states.
    /// </summary>
    const LookupItemViewModelCollection& Changes() const noexcept
    {
        return m_vChanges;
    }

    static const StringModelProperty ActivateButtonTextProperty;
    const std::wstring& GetActivateButtonText() const { return GetValue(ActivateButtonTextProperty); }
    static const BoolModelProperty CanActivateProperty;
    bool CanActivate() const { return GetValue(CanActivateProperty); }
    void ActivateSelected();

    static const StringModelProperty SaveButtonTextProperty;
    const std::wstring& GetSaveButtonText() const { return GetValue(SaveButtonTextProperty); }
    static const BoolModelProperty CanSaveProperty;
    bool CanSave() const { return GetValue(CanSaveProperty); }
    void SaveSelected();

    static const StringModelProperty ResetButtonTextProperty;
    const std::wstring& GetResetButtonText() const { return GetValue(ResetButtonTextProperty); }
    static const BoolModelProperty CanResetProperty;
    bool CanReset() const { return GetValue(CanResetProperty); }
    void ResetSelected();

    static const StringModelProperty RevertButtonTextProperty;
    const std::wstring& GetRevertButtonText() const { return GetValue(RevertButtonTextProperty); }
    static const BoolModelProperty CanRevertProperty;
    bool CanRevert() const { return GetValue(CanRevertProperty); }
    void RevertSelected();

    static const BoolModelProperty CanCreateProperty;
    bool CanCreate() const { return GetValue(CanCreateProperty); }
    void CreateNew();

    static const BoolModelProperty CanCloneProperty;
    bool CanClone() const { return GetValue(CanCloneProperty); }
    void CloneSelected();

    void DoFrame();

    ra::data::DataModelCollection<ra::data::models::AssetModelBase>& Assets() noexcept { return m_vAssets; }
    const ra::data::DataModelCollection<ra::data::models::AssetModelBase>& Assets() const noexcept { return m_vAssets; }

    ra::data::models::AchievementModel* FindAchievement(ra::AchievementID nId)
    {
        return dynamic_cast<ra::data::models::AchievementModel*>(FindAsset(ra::data::models::AssetType::Achievement, nId));
    }
    const ra::data::models::AchievementModel* FindAchievement(ra::AchievementID nId) const
    {
        return dynamic_cast<const ra::data::models::AchievementModel*>(FindAsset(ra::data::models::AssetType::Achievement, nId));
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the filter category.
    /// </summary>
    static const IntModelProperty FilterCategoryProperty;

    /// <summary>
    /// Gets the filter category.
    /// </summary>
    ra::data::models::AssetCategory GetFilterCategory() const { return ra::itoe<ra::data::models::AssetCategory>(GetValue(FilterCategoryProperty)); }

    /// <summary>
    /// Sets the filter category.
    /// </summary>
    void SetFilterCategory(ra::data::models::AssetCategory nValue) { SetValue(FilterCategoryProperty, ra::etoi(nValue)); }

    class AssetSummaryViewModel : public LookupItemViewModel
    {
    public:
        ra::data::models::AssetType GetType() const { return ra::itoe<ra::data::models::AssetType>(GetValue(ra::data::models::AssetModelBase::TypeProperty)); }
        void SetType(ra::data::models::AssetType nValue) { SetValue(ra::data::models::AssetModelBase::TypeProperty, ra::etoi(nValue)); }

        ra::data::models::AssetCategory GetCategory() const { return ra::itoe<ra::data::models::AssetCategory>(GetValue(ra::data::models::AssetModelBase::CategoryProperty)); }
        void SetCategory(ra::data::models::AssetCategory nValue) { SetValue(ra::data::models::AssetModelBase::CategoryProperty, ra::etoi(nValue)); }

        int GetPoints() const { return GetValue(ra::data::models::AchievementModel::PointsProperty); }
        void SetPoints(int nValue) { SetValue(ra::data::models::AchievementModel::PointsProperty, nValue); }

        ra::data::models::AssetState GetState() const { return ra::itoe<ra::data::models::AssetState>(GetValue(ra::data::models::AssetModelBase::StateProperty)); }
        void SetState(ra::data::models::AssetState nValue) { SetValue(ra::data::models::AssetModelBase::StateProperty, ra::etoi(nValue)); }

        ra::data::models::AssetChanges GetChanges() const { return ra::itoe<ra::data::models::AssetChanges>(GetValue(ra::data::models::AssetModelBase::ChangesProperty)); }
        void SetChanges(ra::data::models::AssetChanges nValue) { SetValue(ra::data::models::AssetModelBase::ChangesProperty, ra::etoi(nValue)); }
    };

    ViewModelCollection<AssetSummaryViewModel>& FilteredAssets() noexcept { return m_vFilteredAssets; }
    const ViewModelCollection<AssetSummaryViewModel>& FilteredAssets() const noexcept { return m_vFilteredAssets; }

    void OpenEditor(const AssetSummaryViewModel* pAsset);

    /// <summary>
    /// The <see cref="ModelProperty" /> for the index of the asset that should be made visible.
    /// </summary>
    static const IntModelProperty EnsureVisibleAssetIndexProperty;

    void MergeLocalAssets();
    static const ra::AchievementID FirstLocalId = 111000001;

private:
    // DataModelCollectionBase::NotifyTarget
    void OnDataModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;
    void OnDataModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args) override;
    void OnDataModelAdded(gsl::index nIndex) override;
    void OnDataModelRemoved(gsl::index nIndex) override;
    void OnDataModelChanged(gsl::index nIndex) override;
    void OnEndDataModelCollectionUpdate() override;

    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

    class FilteredListMonitor : public ViewModelCollectionBase::NotifyTarget
    {
    public:
        AssetListViewModel *m_pOwner = nullptr;
        bool m_bUpdateButtonsPending = false;

        void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
        void OnBeginViewModelCollectionUpdate() noexcept override;
        void OnEndViewModelCollectionUpdate() override;
    };
    FilteredListMonitor m_pFilteredListMonitor;

    ra::data::models::AssetModelBase* FindAsset(ra::data::models::AssetType nType, ra::AchievementID nId);
    const ra::data::models::AssetModelBase* FindAsset(ra::data::models::AssetType nType, ra::AchievementID nId) const;

    bool HasSelection(ra::data::models::AssetType nAssetType) const;
    void GetSelectedAssets(std::vector<ra::data::models::AssetModelBase*>& vSelectedAssets);

    void MergeLocalAssets(std::vector<ra::data::models::AssetModelBase*>& vAssetsToMerge, bool bFullMerge);

    void UpdateTotals();
    void UpdateButtons();
    void DoUpdateButtons();
    std::atomic_bool m_bNeedToUpdateButtons = false;

    bool MatchesFilter(const ra::data::models::AssetModelBase& pAsset);
    void AddOrRemoveFilteredItem(const ra::data::models::AssetModelBase& pAsset);
    gsl::index GetFilteredAssetIndex(const ra::data::models::AssetModelBase& pAsset) const;
    void ApplyFilter();

    ra::data::DataModelCollection<ra::data::models::AssetModelBase> m_vAssets;
    ViewModelCollection<AssetSummaryViewModel> m_vFilteredAssets;

    ra::AchievementID m_nNextLocalId = FirstLocalId;

    LookupItemViewModelCollection m_vStates;
    LookupItemViewModelCollection m_vCategories;
    LookupItemViewModelCollection m_vChanges;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif RA_UI_ASSETLIST_VIEW_MODEL_H