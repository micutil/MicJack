# MicJack (マイク・ジャック) 

CC BY Michio Ono (Micono Utilities) 

**MicJack** は、IchigoJamとつないで、インターネットにつなげて使える MixJuice互換のESP-WROOM-02(以降、ESP02)用のプログラムです。 MixJuiceなどESP02機器に入れて使うことができます。 

[イチゴジャムシピさんのサイトでMicJackに関しての紹介して頂いております。](https://15jamrecipe.jimdo.com/mixjuice/micjack/)


#### MicJackの特徴
- MixJuice v1.3 とほぼ互換の機能
- 簡単なWebサーバー機能
- Web画面経由での仮想キーボード
- Kidspod; に対応したコマンド
- Kidspod; にIchigonQuestのプログラムの受送信
- Tello 操作コマンド

[TELLOについて]
200g以下の軽量トーイドローンの [**TELLO (RYYZE社)** ](https://www.ryzerobotics.com/jp/tello) を操作コマンドに対応しています。また、IchigoJamでTELLO / TELLO EDUをコントロールできるようにした [FruitPonch (フルーツポンチ)](https://na-s.jp/FruitPunch/) の省略操作コマンドにも似せてあります。以下のようなIchigoJamを利用してTelloを操作しているページがありますので参考にして下さい。

- [TELLO EDU x IchigoJam - プログラミングでeスポーツ大会＠鯖江](https://fukuno.jig.jp/2471)
- [ How to fly a drone with programming by IchigoJam! / IchigoDakeでドローン “Tello” をプログラミングする方法！]()
- [ IchigoJam + Tello フライト マニュアル (IchigoJam BASIC編)](https://na-s.jp/FruitPunch/docs/manual_1.1.0_ja.html)



**[更新内容]**
 
v1.2.0,b2

- (新規) Telloドローンのコントロールに対応しました(現行でEdu未対応)。
- (新規) 8個のアクセス先を登録できるようにしました。
- (変更)UDPの仕様をMixJuiceに合わせて少し変更しました。
- (変更)アクセスポイントSSID名をMixJuiceに合わせてMicJackからMJ-XXXXXXXXXXXX に 変更しました (XX.. には MixJuice の Mac アドレスが入ります)。

v1.0.1b2
 
- (新規) MJ GETSLASTの不具合修正

v1.0.1b1

- (新規) MJ IJKBD追加:IchigoJamでキーボードモードを使う場合
-  (新規) MJ GETSHOME追加:GETHOMEのGETS対応版
- (新規) MJ GETSLAST追加:GETLASTのGETS対応版

### コマンド一覧

|コマンド名 |機能|
|----------------|-------|
| MJ APC / TJ APC / FP APC | WiFiアクセスポイントに接続 || MJ APD / TJ APD / FP APD | Wi-Fi接続を切断 | | MJ APL / TJ APL / FP APL | WiFiアクセスポイントの一覧を表示 || MJ APS / TJ APS / FP APS | WiFiアクセスポイントへの接続を確認 || MJ SSID | WiFiアクセスポイントのSSIDの表示、デフォルトのSSIDを設定 || MJ SOFTAP | SoftAPのSSID名とIPを表示、SSIDとパスワードを設定| MJ PWD | デフォルトのSSIDのパスワードを設定| MJ RGA / TJ RGA | WiFiアクセスポイントのSSIDとパスワードを登録| MJ RGC / TJ RGC | RGAで登録したWiFiアクセスポイントに登録番号で接続| MJ RGL / TJ RGL | REGAPで登録したWiFiアクセスポイントの一覧を表示| MJ RGD / TJ RGD | RGAで登録したWiFiアクセスポイントを消去| MJ LIP | ローカルIPを表示| MJ GET | HTTPでGET通信| MJ GETS | HTTPSでGET通信| MJ GETHOME | 設定したデフォルトページをGET通信| MJ GETSHOME | 設定したデフォルトページをGETS通信| MJ GETLAST | 最後に通信したページをGET通信| MJ GETSLAST | 最後に通信したページをGETS通信| MJ PROXY | Proxyの設定および表示| MJ PORT | ポート番号の設定および表示| MJ POST ... | HTTPでPOST送信| MJ POSTS ... | HTTPSでPOST送信| MJ PCT | HTTPSでPOST送信でのコンテントタイプを指定| MJ GKP | Kidspod.clubからHTTPでGET通信| MJ PKP | Kidspod.clubにHTTPでPOST送信| MJ QGKP | kidspod.clubからIchigonQuestのプログラムをHTTPでGET通信| MJ QPKP | Kidspod.clubにIchigonQuestのプログラムをHTTPでPOST送信| MJ QSEND | chigonQuestのHEXプログラムを送信します。| MJ UART | HTTPでの入力操作でUARTで送信| MJ KBD | HTTPでの入力操作でキーボード信号として送信| MJ IJKBD | IchigoJamでHTTPでの入力操作でキーボード信号として送信する場合| MJ SPW | HTTPでGET通信において、文字送信遅延の設定| MJ MACADDR | MACアドレスを表示| MJ MAC | MACアドレスを表示(MixJuiceと同じ表示形式)| MJ SLEEP | スリープ(節電モード)|| MJ PMODE | ESP-WROOM-02のピンモード設定| MJ DWRT| ESP-WROOM-02でdigitalWriteを実行| MJ DREAD | ESP-WROOM-02でdigitalReadを実行| MJ AWRT | ESP-WROOM-02でanalogWriteを実行| MJ AREAD | ESP-WROOM-02でanalogReadを実行| MJ MJVER | MicJackのバージョン番号を表示| MJ SVR | サーバーを起動| MJ UDP START | UDPを起動| MJ UDP STOP | UDPを停止| MJ UDP MSG | 最後に使ったリモートIP/PortにデータをUDPで送信| MJ UDP | 指定したリモートIP/PortにデータをUDPで送信| TJ INIT / FP INIT / TJ START / FP START | Tello操作を開始| TJ CLOSE | Tello操作を終了| TJ STATE | Telloの情報を取得| TJ RESON | Telloのコマンド実行後の結果を表示| TJ RESOFF | Telloのコマンド実行後の結果を非表示| TJ Qxxx | FPに準じたTelloのQueueコマンドを送信| TJ xxxx / TJ S xxxx | Telloのコマンドを送信


## 注意・免責 

- SPIFFSを使用しています。SPIFFSにデーターを転送するためのセットアップを行う必要が あります。以下のURLの「[Arduino IDE に ESP8266 SPIFFS ファイルシステムアップローダー をインストールする方法](https://www.mgo-tec.com/spiffs-filesystem-uploader01-html)」などを参考にしてください。
- ドローンのコントロールは、ぶつけたことによる、ドローンの破損、その他の破損、怪我などに十分気をつけて行って下さい。
- なお、MicJack関連を利用したことによるいかなるトラブルに関して、私は一切の責任を負いません。十分に注意して行ってください。

## ライセンス

CC BY IchigoJam http://ichigoJam.net<br>
CC BY IchigoJam-for-Display-1.2 font http://ijutilities.micutil.com <br>
CC BY IchigoLatte-for-Display font http://il.micutil.com<br>