#!/bin/bash
# 参考: https://github.com/uchan-nos/mikanos-build/blob/master/devenv/build-edk2.sh

set -ex


mkdir -p $BUILD_DIR
cd $BUILD_DIR
if [ ! -d edk2 ]; then
  git clone https://github.com/tianocore/edk2.git
fi

cd $BUILD_DIR/edk2


# TOOL_CHAIN_TAGにCLANG38が定義できる最新のバージョン
#git checkout edk2-stable202302
git checkout edk2-stable202208

# サブモジュールを更新
git submodule update --init --recursive

# Conf/target.txtを生成
source edksetup.sh

# 設定値の変更
sed -i '/ACTIVE_PLATFORM/ s:= .*$:= MikanLoaderPkg/MikanLoaderPkg.dsc:' Conf/target.txt
sed -i '/TARGET_ARCH/ s:= .*$:= X64:' Conf/target.txt
sed -i '/TOOL_CHAIN_TAG/ s:= .*$:= CLANG38:' Conf/target.txt

# MikanLoaderのシンボリックリンクがなければ作成
if [ ! -d "MikanLoaderPkg" ]; then
  ln -s $PROJECT_DIR/MikanLoaderPkg ./
fi

# ベースツールをビルド
make -C BaseTools/Source/C

# ビルド
build