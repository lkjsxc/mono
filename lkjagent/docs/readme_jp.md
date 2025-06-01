# lkjagent
lkjagentは必要最低限の機能を備えたAIエージェントです。小さな（具体的には8b程度の）言語モデルで正常に動作させることを目標としています。

## 特徴
-   **完全ローカルかつプライベート:** ローカルマシン上ですべてが完結します。
-   **jsonとマークアップ:** 記憶はjson形式で保存され、エージェントに渡されるときにマークアップ言語へと変換されます。
-   **コンピュータのような記憶方法:** 
    -   **ram:** 後で書きます
    -   **storage:** 後で書きます

## 構造
lkjagent/
├── README.md
├── docs/
│   └── readme_jp.md   # 日本語ドキュメント
└── src/
    ├── index.ts       # メインエントリーポイント
    ├── memory.json     # メモリの状態
    ├── storage.json     # ストレージの状態
    ├── tool/          # ツール群
    │   ├── ram_set.ts
    │   ├── ram_remove.ts
    │   ├── storage_get.ts
    │   ├── storage_ls.ts
    │   ├── storage_remove.ts
    │   ├── storage_search.ts
    │   └── storage_set.ts
    └── types/         # 型定義
        └── common.ts