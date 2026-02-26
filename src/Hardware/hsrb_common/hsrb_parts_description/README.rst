Overview
++++++++

本パッケージはHSRのロボットモデルのパーツ(センサ類)を管理する。
Gazeboシミュレータ用設定も含む。


推奨動作環境
++++++++++++

- Ubuntu Linux 20.04
- ROS Foxy


フォルダ構成
++++++++++++

- **urdf**

    共通マクロを含んだxacroファイルが直下にある。

- **meshes**

    URDFのパーツ毎のレイアウトに対応したフォルダ構成の中に各パーツのメッシュファイルが置かれる。


ROS2 Gazebo 未サポート
++++++++++++

- asus_xtion
- intel_d435
- mynteye

