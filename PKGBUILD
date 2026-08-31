pkgname=muondetector-gui
pkgver=1.0.0
pkgrel=1
pkgdesc="Muon detector GUI application (git version)"
arch=('x86_64' 'aarch64')

url="https://github.com/MuonPi/muondetector"

license=('LGPL-3.0-or-later')

depends=(
    qt6-base
    qt6-declarative
    qt6-positioning
    qwt
)

makedepends=(
  'git'
  'cmake'
  'gcc'
)

source=("git+$url.git#branch=main")
sha256sums=('SKIP')

# dynamic version from git
pkgver() {
  cd "$srcdir/muondetector"
  printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

prepare() {
  cd "$srcdir/muondetector"
}

build() {
  cd "$srcdir/muondetector"

  cmake -B build -S . \
    -G Ninja \
    -DMUONDETECTOR_BUILD_DAEMON=OFF \
    -DMUONDETECTOR_BUILD_TESTS=OFF \
    -DMUONDETECTOR_BUILD_TCP_DEBUG_CLIENT=OFF \
    -DMUONDETECTOR_BUILD_TCP_DEBUG_SERVER=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DPACKAGING_MODE=ON

  cmake --build build
}

package() {
  cd "$srcdir/muondetector"

  DESTDIR="$pkgdir" cmake --install build --prefix /usr --strip
}
