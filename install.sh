#!/usr/bin/env bash
set -euo pipefail

# TODO: not working via curl

APP_NAME="tellydb"
REPO_URL="https://github.com/aloima/tellydb.git"
INSTALL_DIR="/opt/tellydb"
BUILD_DIR="${INSTALL_DIR}/build"
BIN_TARGET="/usr/local/bin/telly"
SERVICE_NAME="tellydb"
DOCKER_IMAGE="aloima/tellydb"
DOCKER_CONTAINER="tellydb"
DEFAULT_PORT="6379"

OS="$(uname -s)"
ARCH="$(uname -m)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m'

SPINNER_PID=""

print_line() {
  printf "%b\n" "${CYAN}────────────────────────────────────────────────────────────${NC}"
}

info() {
  printf "%b\n" "${BLUE}[INFO]${NC} $1"
}

success() {
  printf "%b\n" "${GREEN}[OK]${NC} $1"
}

warn() {
  printf "%b\n" "${YELLOW}[WARN]${NC} $1"
}

error() {
  printf "%b\n" "${RED}[ERROR]${NC} $1"
}

header() {
  clear || true
  print_line
  printf "%b\n" "${BOLD}${CYAN}                   TellyDB Installer${NC}"
  printf "%b\n" "${DIM}           Server Deployment${NC}"
  print_line
  printf "%b\n" "${MAGENTA} OS   : ${OS}${NC}"
  printf "%b\n" "${MAGENTA} ARCH : ${ARCH}${NC}"
  print_line
  echo
}

command_exists() {
  command -v "$1" >/dev/null 2>&1
}

is_macos() {
  [[ "$OS" == "Darwin" ]]
}

is_linux() {
  [[ "$OS" == "Linux" ]]
}

require_root_linux() {
  if is_linux && [[ "${EUID}" -ne 0 ]]; then
    error "This action must be run as root on Linux."
    echo "Please run:"
    echo "  sudo bash install.sh"
    exit 1
  fi
}

ensure_write_access_macos() {
  if is_macos && [[ ! -w "/usr/local/bin" ]]; then
    warn "/usr/local/bin is not writable by the current user."
    warn "You may be prompted for sudo during installation."
  fi
}

run_cmd() {
  local msg="$1"
  shift

  start_spinner "$msg"
  if "$@" >/tmp/tellydb_install.log 2>&1; then
    stop_spinner 0 "$msg"
  else
    stop_spinner 1 "$msg"
    echo
    error "Command failed: $*"
    echo
    echo "---- Last output ----"
    tail -n 50 /tmp/tellydb_install.log || true
    echo "---------------------"
    exit 1
  fi
}

start_spinner() {
  local message="$1"
  local spin='|/-\'
  local i=0

  printf "%b" "${BLUE}[....]${NC} ${message}"
  (
    while true; do
      i=$(( (i + 1) % 4 ))
      printf "\r%b %s %b" "${BLUE}[${spin:$i:1}]${NC}" "${message}" "${DIM}please wait...${NC}"
      sleep 0.1
    done
  ) &
  SPINNER_PID=$!
  disown "$SPINNER_PID" 2>/dev/null || true
}

stop_spinner() {
  local status="${1:-0}"
  local message="${2:-Done}"

  if [[ -n "${SPINNER_PID}" ]]; then
    kill "${SPINNER_PID}" >/dev/null 2>&1 || true
    wait "${SPINNER_PID}" 2>/dev/null || true
    SPINNER_PID=""
  fi

  if [[ "$status" -eq 0 ]]; then
    printf "\r%b %s%*s\n" "${GREEN}[OK]${NC}" "${message}" 20 ""
  else
    printf "\r%b %s%*s\n" "${RED}[FAIL]${NC}" "${message}" 20 ""
  fi
}

pause() {
  echo
  read -r -p "Press Enter to continue..." _
}

detect_pkg_manager() {
  if is_macos; then
    echo "brew"
    return
  fi

  if command_exists apt-get; then
    echo "apt"
  elif command_exists dnf; then
    echo "dnf"
  elif command_exists yum; then
    echo "yum"
  elif command_exists pacman; then
    echo "pacman"
  elif command_exists apk; then
    echo "apk"
  elif command_exists zypper; then
    echo "zypper"
  else
    echo "unknown"
  fi
}

install_packages() {
  local pkg_manager
  pkg_manager="$(detect_pkg_manager)"

  info "Installing build dependencies..."

  case "$pkg_manager" in
    brew)
      if ! command_exists brew; then
        error "Homebrew is not installed."
        echo "Install Homebrew first from the official site, then rerun this script."
        exit 1
      fi

      run_cmd "Updating Homebrew" brew update
      run_cmd "Installing build dependencies" brew install git cmake gperf openssl jemalloc gmp pkg-config
      ;;
    apt)
      run_cmd "Updating apt package index" apt-get update
      run_cmd "Installing build dependencies" env DEBIAN_FRONTEND=noninteractive apt-get install -y \
        git build-essential cmake gperf \
        libssl-dev libjemalloc-dev libgmp-dev \
        pkg-config ca-certificates
      ;;
    dnf)
      run_cmd "Installing build dependencies" dnf install -y \
        git gcc gcc-c++ make cmake gperf \
        openssl-devel jemalloc-devel gmp-devel \
        pkgconf-pkg-config ca-certificates
      ;;
    yum)
      run_cmd "Installing EPEL (if needed)" yum install -y epel-release
      run_cmd "Installing build dependencies" yum install -y \
        git gcc gcc-c++ make cmake gperf \
        openssl-devel jemalloc-devel gmp-devel \
        pkgconfig ca-certificates
      ;;
    pacman)
      run_cmd "Installing build dependencies" pacman -Sy --noconfirm \
        git base-devel cmake gperf \
        openssl jemalloc gmp ca-certificates
      ;;
    apk)
      run_cmd "Installing build dependencies" apk add --no-cache \
        git build-base cmake gperf \
        openssl-dev jemalloc-dev gmp-dev \
        pkgconf ca-certificates
      ;;
    zypper)
      run_cmd "Refreshing zypper repositories" zypper refresh
      run_cmd "Installing build dependencies" zypper install -y \
        git gcc gcc-c++ make cmake gperf \
        libopenssl-devel jemalloc-devel gmp-devel \
        pkg-config ca-certificates
      ;;
    *)
      error "Unsupported package manager."
      echo "Please install these dependencies manually:"
      echo "  git cmake gperf openssl jemalloc gmp pkg-config"
      exit 1
      ;;
  esac

  success "Dependencies installed."
}

prepare_install_paths() {
  if is_macos; then
    INSTALL_DIR="/usr/local/opt/tellydb"
    BUILD_DIR="${INSTALL_DIR}/build"
    BIN_TARGET="/usr/local/bin/telly"
  fi
}

clone_or_update_repo() {
  local LATEST_TAG=$(git ls-remote --tags "${REPO_URL}" | cut -d/ -f3 | sort -V | tail -n1)

  if [[ -d "${INSTALL_DIR}/.git" ]]; then
    info "Existing repository found in ${INSTALL_DIR}. Updating..."
    run_cmd "Updating repository" git -C "${INSTALL_DIR}" fetch --all --tags
    run_cmd "Switching to latest release ${LATEST_TAG}" git -C "${INSTALL_DIR}" checkout "tags/${LATEST_TAG}"
  else
    info "Cloning repository into ${INSTALL_DIR}..."
    rm -rf "${INSTALL_DIR}"
    run_cmd "Cloning repository as latest release" git clone "${REPO_URL}" "${INSTALL_DIR}" --single-branch --branch "${LATEST_TAG}"
  fi

  success "Repository is ready."
}

build_native() {
  prepare_install_paths
  install_packages
  clone_or_update_repo

  info "Building TellyDB from source..."
  mkdir -p "${BUILD_DIR}"
  cd "${BUILD_DIR}"

  if is_macos; then
    export OPENSSL_ROOT_DIR="$(brew --prefix openssl 2>/dev/null || brew --prefix openssl@3 2>/dev/null || true)"
    export CMAKE_PREFIX_PATH="${OPENSSL_ROOT_DIR}:$(brew --prefix jemalloc):$(brew --prefix gmp)"
  fi

  run_cmd "Configuring project with CMake" cmake ..
  run_cmd "Compiling TellyDB" make -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)" telly

  if [[ ! -f "${BUILD_DIR}/telly" ]]; then
    error "Build finished, but the telly binary was not found."
    exit 1
  fi

  if is_macos; then
    if [[ -w "$(dirname "$BIN_TARGET")" ]]; then
      run_cmd "Installing binary" install -m 0755 "${BUILD_DIR}/telly" "${BIN_TARGET}"
    else
      run_cmd "Installing binary with sudo" sudo install -m 0755 "${BUILD_DIR}/telly" "${BIN_TARGET}"
    fi
  else
    run_cmd "Installing binary" install -m 0755 "${BUILD_DIR}/telly" "${BIN_TARGET}"
  fi

  success "Binary installed to ${BIN_TARGET}"
}

ask_create_service_linux() {
  if ! is_linux; then
    warn "Systemd service creation is only available on Linux."
    echo "On macOS, you can run TellyDB manually:"
    echo "  telly"
    return
  fi

  if ! command_exists systemctl; then
    warn "systemd is not available on this Linux system."
    echo "You can start TellyDB manually with:"
    echo "  telly"
    return
  fi

  echo
  read -r -p "Do you want me to create a systemd service for native installation? [Y/n]: " reply
  reply="${reply:-Y}"

  if [[ "$reply" =~ ^[Yy]$ ]]; then
    create_systemd_service
  else
    warn "Skipped systemd service creation."
    echo
    echo "You can start TellyDB manually with:"
    echo "  telly"
  fi
}

create_systemd_service() {
  require_root_linux
  info "Creating systemd service..."

  cat >/etc/systemd/system/${SERVICE_NAME}.service <<EOF
[Unit]
Description=TellyDB Server
After=network.target

[Service]
Type=simple
WorkingDirectory=${INSTALL_DIR}
ExecStart=${BIN_TARGET}
Restart=always
RestartSec=3
User=root
Group=root
LimitNOFILE=65535

[Install]
WantedBy=multi-user.target
EOF

  run_cmd "Reloading systemd" systemctl daemon-reload
  run_cmd "Enabling service" systemctl enable "${SERVICE_NAME}"
  run_cmd "Starting service" systemctl start "${SERVICE_NAME}"

  success "Systemd service created and started."
  echo
  systemctl --no-pager --full status "${SERVICE_NAME}" || true
}

install_docker_macos() {
  warn "Automatic Docker installation is not handled by this script on macOS."
  echo "Please install Docker Desktop manually, then rerun this script."
  exit 1
}

install_docker_if_missing() {
  if command_exists docker; then
    success "Docker is already installed."
    return
  fi

  warn "Docker is not installed."

  if is_macos; then
    install_docker_macos
  fi

  read -r -p "Would you like to install Docker now? [y/N]: " reply
  reply="${reply:-N}"

  if [[ ! "$reply" =~ ^[Yy]$ ]]; then
    warn "Docker installation skipped."
    return
  fi

  local pkg_manager
  pkg_manager="$(detect_pkg_manager)"

  info "Installing Docker..."

  case "$pkg_manager" in
    apt)
      run_cmd "Updating apt package index" apt-get update
      run_cmd "Installing Docker prerequisites" env DEBIAN_FRONTEND=noninteractive apt-get install -y ca-certificates curl gnupg lsb-release
      install -m 0755 -d /etc/apt/keyrings
      curl -fsSL "https://download.docker.com/linux/$(. /etc/os-release && echo "$ID")/gpg" | gpg --dearmor -o /etc/apt/keyrings/docker.gpg
      chmod a+r /etc/apt/keyrings/docker.gpg
      echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/$(. /etc/os-release && echo "$ID") $(. /etc/os-release && echo "$VERSION_CODENAME") stable" >/etc/apt/sources.list.d/docker.list
      run_cmd "Refreshing apt repositories" apt-get update
      run_cmd "Installing Docker Engine" env DEBIAN_FRONTEND=noninteractive apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
      ;;
    dnf)
      run_cmd "Installing DNF plugins" dnf -y install dnf-plugins-core
      run_cmd "Adding Docker repository" dnf config-manager --add-repo https://download.docker.com/linux/fedora/docker-ce.repo
      run_cmd "Installing Docker Engine" dnf install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
      ;;
    yum)
      run_cmd "Installing yum-utils" yum install -y yum-utils
      run_cmd "Adding Docker repository" yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
      run_cmd "Installing Docker Engine" yum install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
      ;;
    pacman)
      run_cmd "Installing Docker" pacman -Sy --noconfirm docker docker-compose
      ;;
    apk)
      run_cmd "Installing Docker" apk add --no-cache docker docker-cli-compose
      ;;
    zypper)
      run_cmd "Installing Docker" zypper install -y docker docker-compose
      ;;
    *)
      error "Unsupported package manager for automatic Docker installation."
      exit 1
      ;;
  esac

  systemctl enable --now docker 2>/dev/null || service docker start 2>/dev/null || true

  if ! command_exists docker; then
    error "Docker installation appears to have failed."
    exit 1
  fi

  success "Docker installed successfully."
}

docker_deploy() {
  if is_linux; then
    require_root_linux
  fi

  install_docker_if_missing

  if ! command_exists docker; then
    error "Docker is still unavailable."
    exit 1
  fi

  run_cmd "Pulling Docker image ${DOCKER_IMAGE}" docker pull "${DOCKER_IMAGE}"

  if docker ps -a --format '{{.Names}}' | grep -qx "${DOCKER_CONTAINER}"; then
    warn "A container named '${DOCKER_CONTAINER}' already exists."
    read -r -p "Do you want to remove and recreate it? [y/N]: " reply
    reply="${reply:-N}"

    if [[ "$reply" =~ ^[Yy]$ ]]; then
      run_cmd "Removing existing container" docker rm -f "${DOCKER_CONTAINER}"
    else
      error "Deployment cancelled to avoid overwriting the existing container."
      exit 1
    fi
  fi

  read -r -p "Port to expose [${DEFAULT_PORT}]: " port
  port="${port:-$DEFAULT_PORT}"

  run_cmd "Starting TellyDB container" docker run -d \
    --name "${DOCKER_CONTAINER}" \
    --restart unless-stopped \
    -p "${port}:6379" \
    "${DOCKER_IMAGE}"

  success "Docker container deployed successfully."
  echo
  echo "Container name : ${DOCKER_CONTAINER}"
  echo "Docker image   : ${DOCKER_IMAGE}"
  echo "Port mapping   : ${port}:6379"
  echo
  docker ps --filter "name=${DOCKER_CONTAINER}"
}

native_install_flow() {
  if is_linux; then
    require_root_linux
  fi

  ensure_write_access_macos
  build_native
  ask_create_service_linux

  echo
  success "Native installation completed."
  echo "Binary path : ${BIN_TARGET}"
  echo "Project dir : ${INSTALL_DIR}"
}

docker_choice_menu() {
  echo
  printf "%b\n" "${BOLD}Docker is already installed on this system.${NC}"
  echo "Choose an installation method:"
  echo
  echo "  1) Docker deployment"
  echo "  2) Native build"
  echo "  3) Exit"
  echo

  while true; do
    read -r -p "Enter your choice [1-3]: " choice
    case "$choice" in
      1)
        docker_deploy
        break
        ;;
      2)
        native_install_flow
        break
        ;;
      3)
        warn "Installer exited."
        exit 0
        ;;
      *)
        error "Invalid choice. Please enter 1, 2, or 3."
        ;;
    esac
  done
}

main() {
  header

  echo "This script installs TellyDB in one of two ways:"
  echo "  - Native build from source"
  echo "  - Docker container deployment"
  echo

  if is_macos; then
    warn "macOS detected."
    echo "Notes:"
    echo "  - Native build uses Homebrew dependencies"
    echo "  - systemd service creation is skipped on macOS"
    echo "  - Docker installation on macOS should be done via Docker Desktop"
    echo
  fi

  if command_exists docker; then
    docker_choice_menu
  else
    warn "Docker was not detected on this system."
    echo "Per your requested logic, the installer will continue with native installation."
    echo
    native_install_flow
  fi

  echo
  print_line
  success "All done."
  echo "To check the binary manually:"
  echo "  telly help"
  print_line
}

main "$@"
