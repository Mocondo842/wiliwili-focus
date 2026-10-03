# wiliwili-focus

本分支（`focus`）基于上游 [`xfangfang/wiliwili`](https://github.com/xfangfang/wiliwili)
的 `yoga` 分支（v1.6.0，commit `88e5876b`），只做「去推荐化」：

> 把**系统推荐内容的显示**摘掉，不删任何内部接口 —— 组件类、`registerXMLView`
> 注册、布局文件、API 层、i18n 词条一律保留，恢复时注释掉相应入口即可。

## 改动

| 提交 | 内容 |
|---|---|
| 主界面 | 移除首页推荐 Tab（保留 动态 / 我的）；搜索入口改为右下角浮动按钮，Y 键与键盘快捷键一并补偿 |
| 搜索页 | 移除热搜榜（普通搜索页与 TV 搜索页）；默认落点仍为「视频」结果页 |
| 播放页 | 拦截「相关推荐」与番剧「推荐」Tab 的创建 |
| 日志 | 关闭 GA 上报；GLFW 平台特性提示降级为 debug；`HighlightProgress` 失败补上 cid/状态码 |

播放页的两处是一个函数入口 `return`，其余代码原样保留。

## 与上游同步

`origin/yoga` 保持与上游一致（GitHub 的 *Sync fork*，或本地 `git fetch`），
本分支只在其上叠加自己的提交，上游更新按常规 merge 合并：

```bash
git fetch origin
git merge origin/yoga
```

为把冲突面压到最小，本分支刻意遵守两条约定：

1. **不改任何 submodule 指针。** 需要修的 borealis 行为（GLFW 错误回调把
   `GLFW_FEATURE_UNAVAILABLE` 当成 ERROR）改由 `wiliwili/source/main.cpp`
   在 `Application::init()` 之后接管回调实现，`library/*` 一行未动，
   因此上游更新 submodule 不会与本分支冲突。
2. **改动尽量落在叶子位置**：删布局节点、给入口加 `return`、包一层 `try/catch`，
   不改函数签名、公共头文件与调用约定。

唯一的例外是 XML 布局本身：上游若改动同一段布局，需要手工合并。

## 构建（Linux x86_64）

```bash
cmake -B build-x64 -DPLATFORM_DESKTOP=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-x64 -j$(nproc)
# 产物 build-x64/wiliwili（资源已一并拷贝到 build-x64/resources）
```

打 deb：

```bash
cmake -B build-x64 -DPLATFORM_DESKTOP=ON -DCMAKE_BUILD_TYPE=Release \
      -DINSTALL=ON -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build-x64 -j$(nproc)
DESTDIR=/tmp/stage cmake --install build-x64
# 再按 deb 规范补 DEBIAN/control 后 dpkg-deb --build --root-owner-group
```

依赖：`libssl-dev libmpv-dev libwebp-dev`（cpr 的 zlib-ng / curl / mongoose
由 CMake 的 FetchContent 拉取）。

## 从 GitHub Actions 取 deb（amd64 / arm64）

`.github/workflows/linux-deb.yaml` 在两个**原生** runner 上同时产出桌面版 deb：

| 架构 | runner | 说明 |
|---|---|---|
| amd64 | `ubuntu-24.04` | |
| arm64 | `ubuntu-24.04-arm` | GitHub 自带的 arm64 机器，公开仓库免费；不交叉编译、不用 QEMU |

每个架构都会把 deb 真装一遍、在 xvfb 下跑 20 秒（`scripts/deb/desktop/smoke.sh`），
跑不起来这一次构建就是红的 —— 所以「绿」同时意味着「装得上、起得来」。

触发：向 `focus` 分支 push（仅限源码/工作流相关路径，改文档不会触发），
或在 Actions 页手动 `Run workflow`（可指定版本号）。

产物在运行页的 artifact 里：`wiliwili_<版本>_amd64.deb` / `wiliwili_<版本>_arm64.deb`。
版本号默认是 `<CMakeLists 里的版本>+focus.<短 SHA>`，例如 `1.6.0+focus.d32936c`。

打包与冒烟逻辑是普通脚本，本机 docker 里同样能跑（需要 `dpkg-deb` 与 `binutils`）：

```bash
DESTDIR="$PWD/stage" cmake --install build-x64
bash scripts/deb/desktop/build.sh stage amd64 1.6.0+local ./dist
bash scripts/deb/desktop/smoke.sh dist/*.deb 20
```
