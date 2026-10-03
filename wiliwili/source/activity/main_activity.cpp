/*
    Copyright 2020-2021 natinusala

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <borealis/core/touch/tap_gesture.hpp>

#include "activity/main_activity.hpp"
#include "utils/activity_helper.hpp"
#include "utils/dialog_helper.hpp"
#include "view/custom_button.hpp"
#include "view/auto_tab_frame.hpp"
#include "view/svg_image.hpp"
#include "fragment/home_tab.hpp"
#include "utils/shortcut_helper.hpp"

using namespace brls::literals;

MainActivity::~MainActivity() { brls::Logger::debug("del MainActivity"); }

void MainActivity::onContentAvailable() {
    // v6 去推荐化：首页 Tab 被移除后，其 Y 键搜索快捷方式在此补偿（键盘快捷键一并补）
    this->registerAction(
        "wiliwili/search/tab"_i18n, brls::ControllerButton::BUTTON_Y,
        [](brls::View* view) -> bool {
            HomeTab::openSearch();
            return true;
        },
        true);

    this->registerAction(ShortcutHelper::getSearch(), [](brls::View* view) -> bool {
        HomeTab::openSearch();
        return true;
    });

    this->registerAction(
        "Settings", brls::ControllerButton::BUTTON_BACK,
        [](brls::View* view) -> bool {
            Intent::openSetting();
            return true;
        },
        true);

    this->registerAction(
        "Settings", brls::ControllerButton::BUTTON_START,
        [](brls::View* view) -> bool {
            Intent::openSetting();
            return true;
        },
        true);

    this->settingBtn->registerClickAction([](brls::View* view) -> bool {
        Intent::openSetting();
        return true;
    });

    this->settingBtn->getFocusEvent()->subscribe([this](bool value) {
        SVGImage* image = dynamic_cast<SVGImage*>(this->settingBtn->getChildren()[0]);
        if (!image) return;
        if (value) {
            image->setImageFromSVGRes("svg/ico-setting-activate.svg");
        } else {
            image->setImageFromSVGRes("svg/ico-setting.svg");
        }
    });

    this->inboxBtn->setCustomNavigation([this](brls::FocusDirection direction) {
        if (tabFrame->getSideBarPosition() == AutoTabBarPosition::LEFT) {
            if (direction == brls::FocusDirection::RIGHT) {
                return (brls::View*)this->tabFrame->getActiveTab();
            } else if (direction == brls::FocusDirection::UP) {
                return (brls::View*)this->tabFrame->getSidebar();
            }
        } else if (tabFrame->getSideBarPosition() == AutoTabBarPosition::TOP) {
            if (direction == brls::FocusDirection::DOWN) {
                return (brls::View*)this->tabFrame->getActiveTab();
            } else if (direction == brls::FocusDirection::LEFT) {
                return (brls::View*)this->tabFrame->getSidebar();
            }
        }
        return (brls::View*)nullptr;
    });
    this->settingBtn->setCustomNavigation([this](brls::FocusDirection direction) {
        if (tabFrame->getSideBarPosition() == AutoTabBarPosition::LEFT) {
            if (direction == brls::FocusDirection::RIGHT) {
                return (brls::View*)this->tabFrame->getActiveTab();
            } else if (direction == brls::FocusDirection::UP) {
                return (brls::View*)this->inboxBtn;
            } else if (direction == brls::FocusDirection::DOWN) {
                return (brls::View*)this->searchBtn;
            }
        } else if (tabFrame->getSideBarPosition() == AutoTabBarPosition::TOP) {
            if (direction == brls::FocusDirection::DOWN) {
                return (brls::View*)this->tabFrame->getActiveTab();
            } else if (direction == brls::FocusDirection::LEFT) {
                return (brls::View*)this->inboxBtn;
            } else if (direction == brls::FocusDirection::RIGHT) {
                return (brls::View*)this->searchBtn;
            }
        }
        return (brls::View*)nullptr;
    });
    this->settingBtn->addGestureRecognizer(new brls::TapGestureRecognizer(this->settingBtn));

    // v6 去推荐化：新的搜索按钮，复用既有的 HomeTab::openSearch()（含 TV / 非 TV 分支）
    this->searchBtn->registerClickAction([](brls::View* view) -> bool {
        HomeTab::openSearch();
        return true;
    });

    this->searchBtn->getFocusEvent()->subscribe([this](bool value) {
        SVGImage* image = dynamic_cast<SVGImage*>(this->searchBtn->getChildren()[0]);
        if (!image) return;
        if (value) {
            image->setImageFromSVGRes("svg/ico-search-activate.svg");
        } else {
            image->setImageFromSVGRes("svg/ico-search.svg");
        }
    });

    this->searchBtn->setCustomNavigation([this](brls::FocusDirection direction) {
        if (tabFrame->getSideBarPosition() == AutoTabBarPosition::LEFT) {
            if (direction == brls::FocusDirection::RIGHT) {
                return (brls::View*)this->tabFrame->getActiveTab();
            } else if (direction == brls::FocusDirection::UP) {
                return (brls::View*)this->settingBtn;
            }
        } else if (tabFrame->getSideBarPosition() == AutoTabBarPosition::TOP) {
            if (direction == brls::FocusDirection::DOWN) {
                return (brls::View*)this->tabFrame->getActiveTab();
            } else if (direction == brls::FocusDirection::LEFT) {
                return (brls::View*)this->settingBtn;
            }
        }
        return (brls::View*)nullptr;
    });
    this->searchBtn->addGestureRecognizer(new brls::TapGestureRecognizer(this->searchBtn));

    this->inboxBtn->registerClickAction([](brls::View* view) -> bool {
        if (DialogHelper::checkLogin()) Intent::openInbox();
        return true;
    });

    this->inboxBtn->getFocusEvent()->subscribe([this](bool value) {
        SVGImage* image = dynamic_cast<SVGImage*>(this->inboxBtn->getChildren()[0]);
        if (!image) return;
        if (value) {
            image->setImageFromSVGRes("svg/ico-inbox-activate.svg");
        } else {
            image->setImageFromSVGRes("svg/ico-inbox.svg");
        }
    });
    this->inboxBtn->addGestureRecognizer(new brls::TapGestureRecognizer(this->inboxBtn));
}