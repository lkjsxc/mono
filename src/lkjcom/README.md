# C言語 HTTPサーバーコア

## 1. プロジェクト概要

このプロジェクトは、C言語で基礎的、モジュール式で、適度に機能豊富な HTTP/1.1 サーバーを作成することを目的とします。サーバーは、明瞭性、堅牢性、そして優れたCプログラミング作法への準拠を念頭に設計されるべきです。静的ファイルの配信と、基本的なGETおよびPOSTリクエストの処理に焦点を当てます。並行処理はPOSIXスレッドを使用して実現されます。

## 2. コア機能

*   **HTTP/1.1 準拠 (基本):**
    *   HTTP GETおよびPOSTリクエストの解析。
    *   一般的なヘッダー（Host, Content-Type, Content-Length, Connection）のサポート。
    *   有効なHTTP/1.1レスポンスの生成。
    *   Keep-Alive接続の処理。
*   **リクエスト処理:**
    *   設定可能なドキュメントルートからの静的ファイルの配信。
    *   ディレクトリリクエストに対する `index.html` の処理。
    *   ファイル拡張子に基づく基本的なMIMEタイプ検出。
    *   パスの基本的なURLデコードのサポート。
*   **POSTリクエスト処理:**
    *   POSTデータ（例：フォームからのデータ）の受信機能。
    *   単純化のため、初期段階では、受信したPOSTデータをレスポンスでそのまま返すか、ログに記録するだけにすることができます。
*   **エラー処理:**
    *   エラー（例：ファイルが見つからない、アクセス権限がない、不正なリクエスト）の丁寧な処理。
    *   適切なHTTPステータスコード（200, 400, 403, 404, 500, 501）の返却。
    *   カスタマイズ可能なエラーページ（例：`404.html`, `500.html`）。
*   **並行性:**
    *   POSIXスレッドを使用して複数のクライアント接続を同時に処理（接続ごとに1スレッド）。
*   **設定:**
    *   サーバーポート（例：コマンドライン引数または単純な設定ファイルから）。
    *   ドキュメントルートディレクトリ（例：コマンドライン引数または単純な設定ファイルから）。
*   **ロギング:**
    *   `stdout` または `stderr` への基本的なログ記録（例：受信リクエスト、エラー、サーバーの起動/停止）。フォーマット: `[タイムスタンプ] [ログレベル] メッセージ`。
*   **セキュリティ (基本的な考慮事項):**
    *   ディレクトリトラバーサル攻撃の防止（リクエストされたパスがドキュメントルート内にあることを確認）。
    *   パスに対する基本的な入力サニタイズ。
    *   バッファオーバーフローに注意する。

## 3. プロジェクト構造

プロジェクトは以下のファイル構造で構成されるべきです:

```
.
├── Makefile
├── .gitignore
├── README.md
├── include/
│   ├── http_parser.h
│   ├── json_parser.h  # (訳注: 元のテキストにjson_parser.h/cがあるが、本文説明では触れられていない。必要なら追加する)
│   ├── http_response.h
│   ├── request_handler.h
│   ├── server_socket.h
│   ├── file_handler.h
│   ├── mime_types.h
│   ├── config.h
│   └── utils.h
├── src/
│   ├── main.c
│   ├── json_parser.c # (訳注: 同上)
│   ├── http_parser.c
│   ├── http_response.c
│   ├── request_handler.c
│   ├── server_socket.c
│   ├── file_handler.c
│   ├── mime_types.c
│   ├── config.c
│   └── utils.c
└── routes/             # デフォルトのドキュメントルート
    ├── index.html      # サンプルインデックスページ
    ├── style.css       # サンプルCSS
    ├── script.js       # サンプルJS
    ├── image.png       # サンプル画像
    ├── 404.html        # カスタム404エラーページ
    └── 500.html        # カスタム500エラーページ
```

### モジュールの責務:

*   **`main.c`**:
    *   コマンドライン引数（ポート、ドキュメントルート）を解析するか、設定を読み込みます。
    *   サーバーソケットを初期化します。
    *   メインのacceptループに入り、新しい接続をハンドラスレッドにディスパッチします。
    *   丁寧なシャットダウンを処理します（例：SIGINT受信時）。
*   **`server_socket.c/.h`**:
    *   サーバーソケットの作成、バインド、リッスンを行う関数。
    *   新しいクライアント接続を受け入れる関数。
*   **`http_parser.c/.h`**:
    *   クライアントソケットからの受信HTTPリクエスト文字列を解析する関数。
    *   メソッド（GET, POST）、URI、HTTPバージョン、ヘッダー、ボディを抽出します。
    *   解析された情報を構造化された方法（例：`HttpRequest`構造体）で保存します。
    *   パスの基本的なURLデコードを処理します。
*   **`request_handler.c/.h`**:
    *   解析されたHTTPリクエストを処理するためのコアロジック。
    *   各クライアント接続スレッドはここから開始されます。
    *   リクエストのタイプ（例：静的ファイルのGET、POST）を決定します。
    *   `file_handler.c`や他のモジュールから適切な関数を呼び出します。
    *   `http_response.c`を使用してレスポンス生成を調整します。
*   **`http_response.c/.h`**:
    *   HTTPレスポンスを構築して送信する関数。
    *   ステータスライン、ヘッダー、ボディを構築します。
    *   クライアントソケットにデータを送信します。
    *   標準エラーレスポンス（400, 403, 404, 500, 501）を送信するヘルパー関数。
*   **`file_handler.c/.h`**:
    *   静的ファイルを配信する関数。
    *   ドキュメントルートからの相対パスでファイルパスを解決します。
    *   セキュリティチェック（例：ディレクトリトラバーサルの防止）を実行します。
    *   ファイルの内容を読み込み、そのサイズを決定します。
    *   `mime_types.c`を使用して`Content-Type`を取得します。
    *   ディレクトリがリクエストされた場合、`index.html`を処理します。
*   **`mime_types.c/.h`**:
    *   ファイル拡張子をMIMEタイプにマッピングする関数（例：".html" -> "text/html"）。
    *   単純なハードコードされたルックアップテーブルを使用できます。
*   **`config.c/.h`**:
    *   サーバー設定（ポート、ドキュメントルート）を読み込む関数。
    *   コマンドライン引数の解析から開始し、後で単純な設定ファイルに拡張することも可能です。
    *   設定値を保持する構造体を定義します。
*   **`utils.c/.h`**:
    *   ユーティリティ関数:
        *   文字列操作ヘルパー（標準ライブラリを超えるものが必要な場合）。
        *   ロギング関数（例：`log_info()`、`log_error()`、`log_debug()`）。
        *   安全なメモリ割り当てラッパー（例：`safe_malloc`、`safe_realloc`）。
        *   URLデコード。

## 4. ビルド手順

*   `Makefile` を提供する必要があります。
*   `src/` ディレクトリ内のすべての `.c` ファイルをコンパイルし、実行可能ファイル（例：`c_http_server`）にリンクする必要があります。
*   コンパイラフラグ: `-Wall -Wextra -pedantic -std=c11 -g` （開発用）。`-pthread` でリンクします。
*   コンパイル済みファイルを削除するための `clean` ターゲット。
*   プロジェクトをビルドするための `all` ターゲット（デフォルト）。

## 5. 使用方法

サーバーはコマンドラインから実行可能であるべきです:

```bash
./c_http_server [ポート] [ドキュメントルート]
```
例:
```bash
./c_http_server 8080 ./routes
```
引数が指定されない場合は、適切なデフォルト値（例：ポート8080、ドキュメントルート `./routes`）を使用します。

## 6. 依存関係

*   標準Cライブラリ (`stdio.h`, `stdlib.h`, `string.h`, `errno.h` など)
*   POSIXライブラリ:
    *   `sys/socket.h`, `netinet/in.h`, `arpa/inet.h` (ソケットプログラミング用)
    *   `unistd.h` (`read`, `write`, `close`, `fork` 用 - ただしスレッドを使用)
    *   `pthread.h` (スレッド用)
    *   `sys/stat.h`, `fcntl.h` (ファイル操作用)
    *   `dirent.h` (ディレクトリ一覧表示用、`index.html` を超えて実装する場合)
    *   `signal.h` (丁寧なシャットダウン用)

## 7. 非機能要件

*   **コードの明瞭性:** 特に複雑なロジックに対する十分なコメント。意味のある変数名と関数名。
*   **エラー処理:** すべてのシステムコールとライブラリ関数に対する堅牢なエラーチェック。
*   **メモリ管理:** メモリリークがないこと。動的に割り当てられたすべてのメモリは解放されなければなりません。
*   **移植性:** POSIX準拠を目指すこと。可能な限りプラットフォーム固有の拡張機能を避けること。
*   **モジュール性:** 各モジュールは明確な責務と明確に定義されたインターフェース（ヘッダーファイル）を持つべきです。

## 8. `routes/` 内のサンプルファイル

簡単なプレースホルダーファイルを作成します:

*   **`routes/index.html`**:
    ```html
    <!DOCTYPE html>
    <html>
    <head><title>Welcome!</title><link rel="stylesheet" href="style.css"></head>
    <body><h1>Hello from C HTTP Server!</h1><p>This is a test page.</p>
    <img src="image.png" alt="sample image" width="100">
    <form action="/submit_form" method="post">
        <label for="name">Name:</label><input type="text" id="name" name="name"><br>
        <label for="data">Data:</label><input type="text" id="data" name="data"><br>
        <input type="submit" value="Submit POST">
    </form>
    <script src="script.js"></script>
    </body></html>
    ```
*   **`routes/style.css`**:
    ```css
    body { font-family: sans-serif; background-color: #f0f0f0; color: #333; }
    h1 { color: #007bff; }
    ```
*   **`routes/script.js`**:
    ```javascript
    console.log("Sample JavaScript loaded!");
    alert("Hello from JavaScript on the C HTTP Server!");
    ```
*   **`routes/404.html`**:
    ```html
    <!DOCTYPE html><html><head><title>404 Not Found</title></head>
    <body><h1>404 - Page Not Found</h1><p>Sorry, the page you are looking for does not exist.</p></body></html>
    ```
*   **`routes/500.html`**:
    ```html
    <!DOCTYPE html><html><head><title>500 Internal Server Error</title></head>
    <body><h1>500 - Internal Server Error</h1><p>Sorry, something went wrong on our end.</p></body></html>
    ```
*   小さなダミーの `routes/image.png` を作成します（どんな小さなPNGファイルでも構いません）。

---

**訳注:**
*   元のテキストのプロジェクト構造に `json_parser.h` と `json_parser.c` が記載されていましたが、モジュールの責務の説明には含まれていませんでした。翻訳ではファイル構造に含め、コメントでその旨を注記しました。
*   技術用語は一般的なカタカナ表記または日本語訳を採用しました。
*   ファイル名、コマンド、コードスニペットは原文のままです。