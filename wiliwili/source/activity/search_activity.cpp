/**
 * Created by fang on 2022/6/9.
 */

#include <borealis/core/touch/tap_gesture.hpp>
#include <borealis/core/bind.hpp>

#include "activity/search_activity.hpp"
#include "fragment/search_tab.hpp"
#include "fragment/search_hots.hpp"
#include "fragment/search_history.hpp"
#include "fragment/search_order.hpp"
#include "utils/config_helper.hpp"
#include "utils/event_helper.hpp"
#include "analytics.h"
#include "utils/shortcut_helper.hpp"

using namespace brls::literals;

SearchActivity::SearchActivity(const std::string& key) {
    SearchActivity::currentKey = key;
    brls::Logger::debug("SearchActivity: create {}", key);
    GA("open_search", {{"key", key}})
}

void SearchActivity::onContentAvailable() {
    brls::Logger::debug("SearchActivity: onContentAvailable");

    auto openText = [this]() {
        brls::Application::getImeManager()->openForText([&](const std::string& text) { this->search(text); },
                                                        "wiliwili/home/common/search"_i18n, "", 32,
                                                        SearchActivity::currentKey, 0);
    };

    this->registerAction("wiliwili/search/tab"_i18n, brls::ControllerButton::BUTTON_Y, [openText](brls::View* view) {
        openText();
        return true;
    });

    this->registerAction(ShortcutHelper::getSearch(), [openText](brls::View* view) {
        openText();
        return true;
    });

    this->searchBox->addGestureRecognizer(new brls::TapGestureRecognizer(this->searchBox, openText));

    this->getUpdateSearchEvent()->subscribe([this](const std::string& s) { this->search(s); });
    // v6 去推荐化：热搜 Tab 已从布局移除。BRLS_BIND 未命中时抛 ViewNotFoundException（不返回 nullptr），
    // 必须捕获异常；捕获后热搜 Tab 的回调自然失效，其余 Tab 不受影响。
    try {
        this->searchTab->getSearchHotsTab()->setSearchCallback(&updateSearchEvent);
    } catch (const brls::ViewNotFoundException& e) {
        brls::Logger::debug("SearchActivity: hot search tab is gone: {}", e.what());
    }
    this->searchTab->getSearchHistoryTab()->setSearchCallback(&updateSearchEvent);

    this->requestSearch(SearchActivity::currentKey);
}

void SearchActivity::search(const std::string& key) {
    if (key.empty()) return;
    SEARCH_E->fire(SEARCH_KEY, (void*)key.c_str());
}

void SearchActivity::requestSearch(const std::string& key) {
    if (key.empty()) return;
    ProgramConfig::instance().addHistory(key);
    SearchActivity::currentKey = key;
    this->labelSearchKey->setText(key);
    // SearchActivity 会最先触发搜索事件，在这里调整页面到默认的搜索页
    // 搜索事件在 SearchActivity 下的其他页面触发时，会根据他当前的显示状态来决定是否立刻进行搜索
    // 由此实现，重新搜索时只加载默认搜索页的结果，减少不必要的网络请求
    this->searchTab->focusNthTab(1);  // v6：热搜 Tab 已删，视频结果页前移为 index 1
    this->searchTab->getSearchVideoTab()->focusNthTab(0);
    this->searchTab->getSearchHistoryTab()->requestHistory();
}

SearchActivity::~SearchActivity() { brls::Logger::debug("SearchActivity: delete"); }

UpdateSearchEvent* SearchActivity::getUpdateSearchEvent() { return &this->updateSearchEvent; }
