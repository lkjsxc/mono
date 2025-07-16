# lkjagent - C言語による自律型AIエージェント

C11標準ライブラリのみを使用して実装された、永続的メモリ管理機能を持ち、LMStudio APIとの連携による複雑なタスク実行が可能な、最小限の自律型AIエージェントです。

## 概要

lkjagentは関数型プログラミングベースのAIエージェントで、複雑なタスクを達成するためのディスクストレージ機能の強化に焦点を当てています。このエージェントは単一のJSONファイルアーキテクチャを通じて揮発性（RAM）と永続的（ディスク）メモリの両方を管理し、ユーザーインターフェースなしで動作します。

## 主な機能

- **C11標準**: 標準ライブラリ関数のみを使用したピュアC実装
- **ゼロ動的割り当て**: 静的メモリ割り当てのみ（malloc/free不使用）
- **関数型プログラミング**: 不変データ構造と純粋関数
- **LMStudio統合**: AI推論のための直接API通信
- **永続的メモリ**: 長期知識保持のためのJSONベースディスクストレージ
- **ステートフル実行**: 明確な状態遷移を持つマルチステートエージェントライフサイクル

## アーキテクチャ

### メモリ管理

エージェントは二重メモリシステムで動作します：

#### RAM（揮発性メモリ）
構造化プロンプトを通じてAIモデルにコンテキストを提供：

- **`system_prompt`**: 固定の行動ガイドラインとエージェント定義
- **`current_state`**: エージェントの現在の動作状態
- **`task_goal`**: 達成すべき最終目標
- **`plan`**: ステップバイステップの実行戦略
- **`scratchpad`**: 一時的なメモとツール実行結果
- **`recent_history`**: 最近のエージェント活動ログ
- **`retrieved_from_disk`**: 永続ストレージから取得した関連知識

#### ディスク（永続的メモリ）
オプションのタグシステムを持つキー値ストア：

- **`working_memory`**: タスク固有の情報とコンテキスト
- **`knowledge_base`**: 蓄積された学習と洞察
- **`log`**: 完全な実行履歴と監査証跡
- **`file`**: 生成されたアーティファクト（コード、文書、データ）
- **`任意のタグ`**: 特別な意味を持たないタグ

### エージェントの状態

エージェントは4つの異なる状態で動作します：

1. **`thinking`**: リクエストを受信し実行計画を策定
2. **`executing`**: 現在の計画に基づいてアクションを実行
3. **`evaluating`**: 結果を評価し次のステップを決定
4. **`paging`**: RAMとディスク間でデータを移動してメモリを管理

### 利用可能なツール

- **`search`**: 関連情報のためのディスクストレージクエリ
- **`retrieve`**: 永続ストレージからの特定データの読み込み
- **`write`**: オプションのタグ付きでディスクへの情報保存
- **`execute_code`**: コードスニペットの実行と結果キャプチャ
- **`forget`**: メモリ最適化のための不要情報の削除

## ビルドとコンパイル

### 前提条件

- C11互換コンパイラ（GCC 4.9+またはClang 3.1+）
- 標準Cライブラリ
- HTTPリクエスト用curlライブラリ（通常プリインストール済み）

### コンパイル

```bash
# 基本コンパイル
gcc -std=c11 -Wall -Wextra -O2 -o lkjagent lkjagent/main.c

# デバッグシンボル付き
gcc -std=c11 -Wall -Wextra -g -DDEBUG -o lkjagent lkjagent/main.c

# プロダクションビルド
gcc -std=c11 -Wall -Wextra -O3 -DNDEBUG -o lkjagent lkjagent/main.c
```

### CMakeビルド（推奨）

```bash
mkdir build
cd build
cmake ..
make
```

## 設定

### LMStudioセットアップ

1. LMStudioをインストールして実行
2. 好みの言語モデルを読み込み
3. ローカルサーバーを開始（通常`http://localhost:1234`）
4. エージェントの設定でAPIエンドポイントを設定

### エージェント設定

`config.json`ファイルを作成：

```json
{
  "lmstudio": {
    "endpoint": "http://localhost:1234/v1/chat/completions",
    "model": "your-model-name",
    "max_tokens": 2048
  },
  "memory": {
    "ram_size": 8192,
    "disk_file": "agent_memory.json",
    "max_history": 100
  },
  "agent": {
    "max_iterations": 50,
    "evaluation_threshold": 0.8
  }
}
```

## 使用方法

### 基本実行

```bash
# 実行
./lkjagent
```

## メモリアーキテクチャの詳細

### JSONストレージ形式

```json
{
  "metadata": {
    "version": "1.0",
    "created": "2025-07-16T00:00:00Z",
    "last_modified": "2025-07-16T12:00:00Z"
  },
  "working_memory": {
    "current_task": "...",
    "context": "...",
    "variables": {}
  },
  "knowledge_base": {
    "concepts": {},
    "procedures": {},
    "facts": {}
  },
  "log": [
    {
      "timestamp": "2025-07-16T12:00:00Z",
      "state": "thinking",
      "action": "plan_task",
      "details": "..."
    }
  ],
  "file": {
    "generated_code": {},
    "documents": {},
    "data": {}
  }
}
```

### 状態遷移

```
[ユーザー入力] → thinking → executing → evaluating
                ↑             ↓           ↓
               paging ←────────────────────┘
```

## 開発

### コード構造

```
lkjagent/
├──data.json                # ram, disk
├──config.json              # 設定
└──src/
    ├── main.c              # エントリーポイントとメインループ
    ├── agent.h             # エージェントコア定義
    ├── agent.c             # エージェント状態管理
    ├── memory.h            # メモリ管理インターフェース
    ├── memory.c            # JSONベースメモリ実装
    ├── tools.h             # ツール定義
    ├── tools.c             # ツール実装
    ├── http.h              # HTTPクライアントインターフェース
    ├── http.c              # LMStudio API通信
    ├── utils.h             # ユーティリティ関数
    └── utils.c
```

### コーディング標準

- **C11標準**: 有益な場合はC11機能を使用
- **動的割り当て禁止**: 静的配列とスタック割り当てのみ
- **関数型スタイル**: 純粋関数と不変データを優先
- **エラーハンドリング**: すべての操作で明示的なエラーコード
- **ドキュメント**: 包括的なインラインドキュメント

### メモリ制約

- **最大JSONサイズ**: 16MB（設定可能）
- **RAMバッファ**: 8KB静的割り当て
- **文字列制限**: 文字列フィールドあたり2KB
- **配列制限**: 最大256要素

## API統合

### LMStudio通信

エージェントはHTTP POSTリクエストを使用してLMStudioと通信：

```c
// APIコール構造の例
typedef struct {
    char* model;
    char* prompt;
    int max_tokens;
} api_request_t;

int call_lmstudio(const api_request_t* request, char* response, size_t response_size);
```

### レスポンス処理

AI応答は以下を抽出するために解析されます：
- 実行すべき次のアクション
- 更新されたエージェント状態
- 保存すべき情報
- ツール呼び出し

## エラーハンドリング

エージェントは包括的なエラーハンドリングを実装：

- **ネットワークエラー**: 指数バックオフでリトライ
- **JSON解析エラー**: グレースフルな劣化
- **メモリエラー**: 安全な失敗モード
- **APIエラー**: フォールバック戦略

## パフォーマンス考慮事項

- **メモリ使用量**: 固定メモリフットプリント
- **ディスクI/O**: 効率的なJSON操作
- **ネットワーク**: コネクションプーリングとキャッシュ
- **CPU**: 最適化された文字列操作

## デバッグ

### デバッグモード

```bash
./lkjagent --debug --verbose
```

デバッグ出力に含まれるもの：
- 状態遷移
- メモリ操作
- APIリクエスト/レスポンス
- ツール実行

### メモリ検査

```bash
# 現在のメモリ状態を表示
cat agent_memory.json | jq '.'

# メモリ変更を監視
watch -n 1 'cat agent_memory.json | jq ".metadata.last_modified"'
```

## 貢献

1. リポジトリをフォーク
2. 機能ブランチを作成
3. コーディング標準に従う
4. 包括的なテストを追加
5. プルリクエストを提出

## ライセンス

MITライセンス - 詳細は[LICENSE](LICENSE)ファイルを参照

## ロードマップ

- [ ] 強化されたツールシステム
- [ ] 複数モデルサポート
- [ ] 分散メモリ
- [ ] パフォーマンス最適化
- [ ] 拡張API互換性

## サポート

問題や質問について：
- GitHub Issues: [イシューを作成](https://github.com/lkjsxc/mono/issues)
- ドキュメント: インラインコードドキュメントを参照
- 例: `examples/`ディレクトリを確認

---

**注意**: このエージェントは自律運用のために設計されています。本番環境でのデプロイ時には適切な監視と安全対策を確実に実施してください。
