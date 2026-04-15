# torabo-tsuki セットアップログ

## 基本情報

- キーボード: torabo-tsuki (M) 42キー
- 入手: メルカリ中古品
- MCU: BLE Micro Pro (nRF52840)
- トラックボール: 右手側
- 前キーボード: Keyball44 (QMK, Remap)

## リポジトリ

| リポジトリ | 用途 |
|---|---|
| [ren-mntn/torabo-tsuki](https://github.com/ren-mntn/torabo-tsuki) | QMK (vial-qmk) カスタムファームウェア |
| [ren-mntn/zmk-keyboard-torabo-tsuki](https://github.com/ren-mntn/zmk-keyboard-torabo-tsuki) | ZMK カスタムファームウェア (移行先) |

## 作業ログ

### 2026-04-09: Keyball44 キーマップ抽出 & QMK移植

1. **Remap からキーマップ抽出**
   - ブラウザでRemapを開き、Redux storeからdevice.keymapsを取得
   - 7レイヤー × 48キー (8行×6列マトリクス) の生キーコードを保存
   - `keyball44_remap_keycodes.json` に保存済み

2. **生キーコードをQMK名にデコード**
   - Pythonスクリプトで全キーコードをQMK名に変換
   - QK_MODS, QK_MOD_TAP, QK_LAYER_TAP, QK_TOGGLE_LAYER, QK_KB 等を解析
   - Keyball44の `keyball.h` でカスタムキーコード対応 (Kb 7 = SCRL_MO 等)

3. **Keyball44 keymap.c を更新**
   - 7レイヤー全てRemapデータから反映
   - ビルド成功: `make SKIP_GIT=yes keyball/keyball44:default` → 22422/28672 bytes (78%)

4. **torabo-tsuki QMK リポジトリ作成**
   - `ren-mntn/torabo-tsuki` (public, GitHub)
   - vial-qmk の torabo-tsuki キーボードファイルを格納
   - `keymaps/mymap/` にカスタムキーマップ作成

5. **QMKカスタムロジック移植**
   - タイピングモード (マウス/タイピング自動切替)
   - BTN1→J, BTN2→L, DRAG_SCROLL→K 変換
   - 母音自動補完 (300ms以内の母音入力で子音自動挿入)
   - 固定スクロールモード (BTN2でスクロール固定)
   - ピンチズーム (スクロール中BTN1→Cmd+クリック)
   - OSキーリピート対応

6. **QMKビルド環境構築**
   - `vial-qmk` フルリポジトリをclone (dev/ble-micro-pro ブランチ)
   - ビルド成功: `.uf2` ファイル生成
   - 書き込み方法: dfuコマンド → ブートローダー → UF2コピー

### 2026-04-10: 実機セットアップ & ZMK移行

1. **初期ペアリング問題**
   - 中古品のため前オーナーのペアリング情報が残っていた
   - 解決: Web Configurator の Edit Config で左右のロール設定が必要だった
     - 右側（マスター）: Is Slave OFF, Is Left OFF
     - 左側（スレーブ）: Is Slave ON, Is Left ON
   - Pair Device1 with New Device で左右ペアリング完了

2. **QMKカスタムファームウェア書き込み**
   - 右側: dfuコマンド (`echo "dfu" > /dev/cu.usbmodemvial_*`) でブートローダー起動
   - UF2をBLEMICROPROボリュームにコピーで書き込み
   - 成功: タイピングモード等のカスタム機能動作確認
   - 再ペアリング: ファームウェア書き込みでEEPROMリセットされるため Edit Config 再設定必要

3. **Vial設定 (via-custom-ui-for-vial)**
   - CPI: 200
   - Scroll Divide: 26
   - Battery Mode: Custom (Performance 3ベース)
   - Connection interval: 8ms (最小)
   - Slave latency: 15
   - Battery type: Ni-MH

4. **BLE接続時のカーソル遅延問題**
   - USB接続時は滑らか、BLE時はフレームレート低い感じ
   - 原因: QMKファームウェアのトラックボール更新間隔 (10-30ms) + macOSのBLE interval制限
   - Battery Mode Performance 3 でもCPI上げても改善しない
   - → ZMK移行を決定 (Zephyrスタックのほうが高頻度BLE通信可能)

5. **ZMK リポジトリ作成**
   - `sekigon-gonnoc/zmk-keyboard-torabo-tsuki` を `ren-mntn` にfork
   - GitHub Actions でビルド (デフォルトfirmware: 成功)

6. **ZMK カスタムBehavior実装**
   - `clk_or_key` behavior: タイピング/マウスモード切替 (2-param: key, mouse_button_bit)
   - `typing_mode.c`: グローバル状態管理 + ZMK_LISTENER (zmk_keycode_state_changed購読)
   - `input_processor_mouse_accel.c`: Keyball44風マウス加速カーブ + タイピングモード解除
   - 7レイヤーキーマップ (M variant, 50キー/レイヤー)
   - GitHub Actions ビルド: 1回目リンカエラー → IS_CENTRAL ガード追加 → 2回目成功

7. **ZMK書き込み問題 (未解決)**
   - 右側: ブートローダー起動可能 (dfuコマンド → BLEMICROPRO ドライブ出現)
   - **左側: USB接続でFinderに出ない**
     - Web Configurator (WebUSB) からは認識される
     - Edit Config でIs Slave/Is Left設定可能
     - Update Application でブートローダーは起動するがマスストレージが出ない
     - シリアルポートも出ない (`/dev/cu.usbmodem*` なし) ← 要再確認
   - 原因: 左BMPのUSBマスストレージクラスが死んでる可能性
   - **次のステップ: adafruit-nrfutil でシリアルDFU経由書き込み or DevTools注入**

## レイヤー構成 (共通: QMK/ZMK)

| Layer | 用途 | アクセス |
|---|---|---|
| 0 | Base QWERTY | デフォルト |
| 1 | エディタショートカット (Alt/Gui combos) | RET長押し |
| 2 | 記号 | SPC長押し |
| 3 | Neovim (Ctrl+H/J/K/L分割移動等) | ESC長押し |
| 4 | 数字 | LNG1長押し |
| 5 | ウィンドウ管理/スクリーンショット (C+S+A combos) | ;(SEMI)長押し |
| 6 | ゲーミング/数字行 (TG(6)トグル) | TG(6) |

## カスタム機能 (Keyball44から移植)

### タイピングモード
- **概要**: トラックボール操作中はマウスモード、キー入力中はタイピングモード
- **BTN1/BTN2/K変換**: タイピングモード中はBTN1→J、BTN2→L、中クリック→K
- **母音自動補完**: クリック後300ms以内に母音(A/I/U/E/O)入力で子音(J/K/L)を自動挿入
  - 例: Jクリック→A入力 → "JA"が入力される
- **OSキーリピート**: タイピングモード中のクリック変換キーはregister/unregisterでOS側キーリピート利用

### マウス加速カーブ
- 指数関数ベース: `min + (max - min) * (1 - exp(-accel * magnitude))`
- min_speed=0.4, max_speed=1.8, acceleration=0.03
- 低速域(< 3.0)は線形スケールで精密操作
- 移動量 > 3.0 でタイピングモード解除 (微振動では解除されない)

### 固定スクロール (QMKのみ, ZMK未実装)
- DRAG_SCROLL中にBTN2 → スクロールモード固定
- 任意キーで固定解除
- トラックボール移動でも解除

### ピンチズーム (QMKのみ, ZMK未実装)
- スクロールモード中のBTN1 → Cmd+クリック (Figmaピンチイン/アウト用)

## ハードウェアメモ

### BLE Micro Pro
- ブートローダー起動: P/Qキー押しながらUSB挿入 or `echo "dfu" > /dev/cu.usbmodem*`
- Web Configurator: https://sekigon-gonnoc.github.io/BLE-Micro-Pro-WebConfigurator/
- Vial設定: https://sekigon-gonnoc.github.io/via-custom-ui-for-vial/
- ZMK Studio: ファームウェアに `CONFIG_ZMK_STUDIO=y` で有効

### BLE接続安定化
- Battery Mode を Performance 2 以上に変更 (via-custom-ui-for-vial)
- Connection interval 8ms, Slave latency 0-15
- 参考: https://x.com/irys33/status/1832418068766875936

### Battery Mode パラメータ (QMK torabo_tsuki.c より)

| Mode | Master Interval | Slave Interval | TB Interval |
|---|---|---|---|
| Standard | 15ms | 30ms | 30ms |
| Performance 1 | 10ms | 20ms | 30ms |
| Performance 2 | 10ms | 10ms | 20ms |
| Performance 3 | 10ms | 10ms | 10ms |
| Custom | ユーザー設定 | ユーザー設定 | 30ms (fallback) |

### SSH/GitHub設定
- ren-mntn アカウント: `github.com-private` SSH host
- SSH鍵: `~/.ssh/id_ed25519_ren_mntn`
- gh CLI: RenMunetsuna がデフォルト、ren-mntn は GH_TOKEN 環境変数で切替

## ビルド手順

### QMK (vial-qmk)
```bash
cd ~/workspace/private/torabo-tsuki/vial-qmk
make SKIP_GIT=yes sekigon/torabo_tsuki:mymap
arm-none-eabi-objcopy -O binary .build/sekigon_torabo_tsuki_mymap.elf .build/sekigon_torabo_tsuki_mymap.bin
python3 util/vial_generate_vfw.py .build/sekigon_torabo_tsuki_mymap.bin .build/sekigon_torabo_tsuki_mymap.vfw keyboards/sekigon/torabo_tsuki/keymaps/mymap/config.h
# UF2生成:
make SKIP_GIT=yes sekigon/torabo_tsuki:mymap:uf2
```

### ZMK
```bash
# リモートビルド (GitHub Actions)
cd ~/workspace/private/zmk-keyboard-torabo-tsuki
git push  # → GitHub Actions が自動でビルド

# 成果物: Actions の "firmware" artifact からダウンロード
# GH_TOKEN=<ren-mntn PAT> gh run download <run_id> --repo ren-mntn/zmk-keyboard-torabo-tsuki --dir /tmp/zmk-firmware
```

### 書き込み
```bash
# ブートローダー起動 (右側)
echo "dfu" > /dev/cu.usbmodemvial_*
# → BLEMICROPRO ドライブが出現

# UF2コピー
cp <firmware>.uf2 /Volumes/BLEMICROPRO/

# 左側: USB認識問題あり → adafruit-nrfutil で DFU 書き込み予定
```
