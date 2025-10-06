#!/usr/bin/env bash
#
# pgbalancer Build Script
# Copyright (c) 2003-2021 PgPool Global Development Group
# Copyright (c) 2025 pgElephant
#
# Comprehensive build script with OS detection and dependency management
#

set -e  # Exit on error
set -u  # Exit on undefined variable

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Build configuration
BUILD_TYPE="${BUILD_TYPE:-release}"
JOBS="${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"
PREFIX="${PREFIX:-/usr/local}"
POSTGRESQL_VERSION="${POSTGRESQL_VERSION:-17}"

# Flags
CLEAN_BUILD="${CLEAN_BUILD:-no}"
SKIP_DEPS="${SKIP_DEPS:-no}"
VERBOSE="${VERBOSE:-no}"

#------------------------------------------------------------------------------
# Utility Functions
#------------------------------------------------------------------------------

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

#------------------------------------------------------------------------------
# OS Detection
#------------------------------------------------------------------------------

detect_os() {
    print_header "Detecting Operating System"
    
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS_NAME="$ID"
        OS_VERSION="$VERSION_ID"
        OS_PRETTY="$PRETTY_NAME"
    elif [ -f /etc/redhat-release ]; then
        OS_NAME="rhel"
        OS_VERSION=$(cat /etc/redhat-release | grep -oE '[0-9]+\.[0-9]+' | head -1)
        OS_PRETTY=$(cat /etc/redhat-release)
    elif [ "$(uname)" = "Darwin" ]; then
        OS_NAME="macos"
        OS_VERSION=$(sw_vers -productVersion)
        OS_PRETTY="macOS $OS_VERSION"
    else
        print_error "Unable to detect operating system"
        exit 1
    fi
    
    print_info "Detected: $OS_PRETTY"
    print_info "OS: $OS_NAME"
    print_info "Version: $OS_VERSION"
}

#------------------------------------------------------------------------------
# Dependency Installation
#------------------------------------------------------------------------------

install_deps_rhel() {
    print_header "Installing Dependencies (RHEL/Rocky/AlmaLinux)"
    
    local pg_major="${POSTGRESQL_VERSION}"
    local packages=(
        "gcc"
        "gcc-c++"
        "make"
        "autoconf"
        "automake"
        "libtool"
        "bison"
        "flex"
        "git"
        "openssl-devel"
        "readline-devel"
        "zlib-devel"
        "libyaml-devel"
        "libmemcached-devel"
        "postgresql${pg_major}-devel"
        "postgresql${pg_major}-server"
    )
    
    print_info "Installing packages: ${packages[*]}"
    
    if command_exists dnf; then
        sudo dnf install -y "${packages[@]}" || print_warning "Some packages may already be installed"
    elif command_exists yum; then
        sudo yum install -y "${packages[@]}" || print_warning "Some packages may already be installed"
    else
        print_error "No package manager found (dnf/yum)"
        exit 1
    fi
    
    # Set PostgreSQL paths
    export PG_CONFIG="/usr/pgsql-${pg_major}/bin/pg_config"
    export PGSQL_INCLUDE="/usr/pgsql-${pg_major}/include"
    export PGSQL_LIB="/usr/pgsql-${pg_major}/lib"
}

install_deps_debian() {
    print_header "Installing Dependencies (Debian/Ubuntu)"
    
    local pg_major="${POSTGRESQL_VERSION}"
    local packages=(
        "build-essential"
        "gcc"
        "g++"
        "make"
        "autoconf"
        "automake"
        "libtool"
        "bison"
        "flex"
        "git"
        "libssl-dev"
        "libreadline-dev"
        "zlib1g-dev"
        "libyaml-dev"
        "libmemcached-dev"
        "postgresql-server-dev-${pg_major}"
        "postgresql-${pg_major}"
    )
    
    print_info "Updating package list"
    sudo apt-get update -qq
    
    print_info "Installing packages: ${packages[*]}"
    sudo apt-get install -y "${packages[@]}" || print_warning "Some packages may already be installed"
    
    # Set PostgreSQL paths
    export PG_CONFIG="/usr/lib/postgresql/${pg_major}/bin/pg_config"
    export PGSQL_INCLUDE="/usr/include/postgresql/${pg_major}/server"
    export PGSQL_LIB="/usr/lib/postgresql/${pg_major}/lib"
}

install_deps_fedora() {
    print_header "Installing Dependencies (Fedora)"
    
    local packages=(
        "gcc"
        "gcc-c++"
        "make"
        "autoconf"
        "automake"
        "libtool"
        "bison"
        "flex"
        "git"
        "openssl-devel"
        "readline-devel"
        "zlib-devel"
        "libyaml-devel"
        "libmemcached-devel"
        "postgresql-devel"
        "postgresql-server"
    )
    
    print_info "Installing packages: ${packages[*]}"
    sudo dnf install -y "${packages[@]}" || print_warning "Some packages may already be installed"
    
    # Set PostgreSQL paths
    export PG_CONFIG="/usr/bin/pg_config"
    export PGSQL_INCLUDE="/usr/include/pgsql/server"
    export PGSQL_LIB="/usr/lib64/pgsql"
}

install_deps_arch() {
    print_header "Installing Dependencies (Arch Linux)"
    
    local packages=(
        "base-devel"
        "gcc"
        "make"
        "autoconf"
        "automake"
        "libtool"
        "bison"
        "flex"
        "git"
        "openssl"
        "readline"
        "zlib"
        "libyaml"
        "libmemcached"
        "postgresql"
    )
    
    print_info "Installing packages: ${packages[*]}"
    sudo pacman -Syu --noconfirm --needed "${packages[@]}"
    
    # Set PostgreSQL paths
    export PG_CONFIG="/usr/bin/pg_config"
    export PGSQL_INCLUDE="/usr/include/postgresql/server"
    export PGSQL_LIB="/usr/lib/postgresql"
}

install_deps_macos() {
    print_header "Installing Dependencies (macOS)"
    
    if ! command_exists brew; then
        print_error "Homebrew not found. Please install from https://brew.sh"
        exit 1
    fi
    
    local packages=(
        "autoconf"
        "automake"
        "libtool"
        "bison"
        "flex"
        "openssl@3"
        "readline"
        "libyaml"
        "libmemcached"
        "postgresql@${POSTGRESQL_VERSION}"
    )
    
    print_info "Installing packages: ${packages[*]}"
    brew install "${packages[@]}" || print_warning "Some packages may already be installed"
    
    # Set PostgreSQL paths
    export PG_CONFIG="/usr/local/opt/postgresql@${POSTGRESQL_VERSION}/bin/pg_config"
    export PGSQL_INCLUDE="/usr/local/opt/postgresql@${POSTGRESQL_VERSION}/include/server"
    export PGSQL_LIB="/usr/local/opt/postgresql@${POSTGRESQL_VERSION}/lib"
    
    # Set OpenSSL paths for macOS
    export LDFLAGS="-L/usr/local/opt/openssl@3/lib"
    export CPPFLAGS="-I/usr/local/opt/openssl@3/include"
}

install_dependencies() {
    if [ "$SKIP_DEPS" = "yes" ]; then
        print_warning "Skipping dependency installation (SKIP_DEPS=yes)"
        return
    fi
    
    case "$OS_NAME" in
        rhel|rocky|almalinux|centos)
            install_deps_rhel
            ;;
        debian|ubuntu)
            install_deps_debian
            ;;
        fedora)
            install_deps_fedora
            ;;
        arch)
            install_deps_arch
            ;;
        macos)
            install_deps_macos
            ;;
        *)
            print_warning "Unsupported OS: $OS_NAME"
            print_warning "Please install dependencies manually"
            ;;
    esac
}

#------------------------------------------------------------------------------
# PostgreSQL Detection
#------------------------------------------------------------------------------

detect_postgresql() {
    print_header "Detecting PostgreSQL"
    
    # Try to find pg_config
    local pg_config_paths=(
        "/usr/pgsql-${POSTGRESQL_VERSION}/bin/pg_config"
        "/usr/lib/postgresql/${POSTGRESQL_VERSION}/bin/pg_config"
        "/usr/local/opt/postgresql@${POSTGRESQL_VERSION}/bin/pg_config"
        "/usr/bin/pg_config"
        "/usr/local/bin/pg_config"
    )
    
    for pg_path in "${pg_config_paths[@]}"; do
        if [ -x "$pg_path" ]; then
            export PG_CONFIG="$pg_path"
            break
        fi
    done
    
    if [ -z "${PG_CONFIG:-}" ]; then
        print_error "pg_config not found. Please install PostgreSQL development packages"
        exit 1
    fi
    
    print_info "Using pg_config: $PG_CONFIG"
    
    # Get PostgreSQL information
    local pg_version=$($PG_CONFIG --version | awk '{print $2}')
    local pg_includedir=$($PG_CONFIG --includedir-server)
    local pg_libdir=$($PG_CONFIG --libdir)
    local pg_bindir=$($PG_CONFIG --bindir)
    
    print_info "PostgreSQL version: $pg_version"
    print_info "Include directory: $pg_includedir"
    print_info "Library directory: $pg_libdir"
    print_info "Binary directory: $pg_bindir"
    
    export PGSQL_INCLUDE="$pg_includedir"
    export PGSQL_LIB="$pg_libdir"
    export PGSQL_BIN="$pg_bindir"
}

#------------------------------------------------------------------------------
# Build Functions
#------------------------------------------------------------------------------

clean_build() {
    print_header "Cleaning Build Artifacts"
    
    if [ -f Makefile ]; then
        print_info "Running 'make clean'"
        make clean || true
    fi
    
    print_info "Removing generated files"
    rm -f configure config.log config.status
    rm -rf autom4te.cache
    rm -f aclocal.m4
    rm -f Makefile.in
    rm -f ltmain.sh
    rm -f m4/libtool.m4 m4/ltoptions.m4 m4/ltsugar.m4 m4/ltversion.m4 m4/lt~obsolete.m4
    
    find . -name "*.o" -delete
    find . -name "*.lo" -delete
    find . -name "*.la" -delete
    find . -name ".deps" -type d -exec rm -rf {} + 2>/dev/null || true
    find . -name ".libs" -type d -exec rm -rf {} + 2>/dev/null || true
    
    print_success "Clean completed"
}

generate_configure() {
    print_header "Generating Configure Script"
    
    if ! command_exists autoreconf; then
        print_error "autoreconf not found. Please install autoconf"
        exit 1
    fi
    
    print_info "Running autoreconf -i"
    autoreconf -i
    
    if [ ! -f configure ]; then
        print_error "Failed to generate configure script"
        exit 1
    fi
    
    print_success "Configure script generated"
}

configure_build() {
    print_header "Configuring Build"
    
    local configure_args=(
        "--prefix=$PREFIX"
        "--with-pgsql=$(dirname $PGSQL_BIN)"
        "--with-pgsql-includedir=$(dirname $PGSQL_INCLUDE)"
        "--with-pgsql-libdir=$PGSQL_LIB"
    )
    
    # Add optional features
    if [ -d /usr/include/yaml.h ] || [ -d /usr/local/include/yaml.h ]; then
        print_info "libyaml detected - enabling YAML support"
    fi
    
    if [ -d /usr/include/libmemcached ] || [ -d /usr/local/include/libmemcached ]; then
        configure_args+=("--with-memcached=/usr")
    fi
    
    # Debug or release build
    if [ "$BUILD_TYPE" = "debug" ]; then
        configure_args+=("--enable-cassert" "--enable-debug")
        print_info "Debug build enabled"
    fi
    
    print_info "Configure arguments: ${configure_args[*]}"
    
    if [ "$VERBOSE" = "yes" ]; then
        ./configure "${configure_args[@]}"
    else
        ./configure "${configure_args[@]}" > configure.log 2>&1
    fi
    
    if [ $? -ne 0 ]; then
        print_error "Configure failed. See configure.log for details"
        tail -50 configure.log
        exit 1
    fi
    
    print_success "Configuration completed"
}

compile_build() {
    print_header "Compiling pgbalancer"
    
    print_info "Building with $JOBS parallel jobs"
    
    if [ "$VERBOSE" = "yes" ]; then
        make -j"$JOBS"
    else
        make -j"$JOBS" > build.log 2>&1
    fi
    
    if [ $? -ne 0 ]; then
        print_error "Build failed. See build.log for details"
        tail -100 build.log
        exit 1
    fi
    
    print_success "Build completed"
}

verify_build() {
    print_header "Verifying Build"
    
    local pgbalancer_bin="src/pgbalancer"
    
    if [ ! -f "$pgbalancer_bin" ]; then
        print_error "pgbalancer binary not found at $pgbalancer_bin"
        exit 1
    fi
    
    print_info "Binary location: $pgbalancer_bin"
    print_info "Binary size: $(du -h $pgbalancer_bin | awk '{print $1}')"
    
    # Test version
    print_info "Testing pgbalancer --version"
    if ./"$pgbalancer_bin" --version 2>&1 | head -5; then
        print_success "pgbalancer binary is working"
    else
        print_error "pgbalancer binary test failed"
        exit 1
    fi
}

organize_build() {
    print_header "Organizing Build Output"
    
    local BUILD_DIR="$SCRIPT_DIR/build"
    
    # Create build directory structure
    print_info "Creating build directory structure"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"/{bin,conf,lib}
    
    # Copy binaries
    print_info "Copying binaries to build/bin/"
    if [ -f "src/pgbalancer" ]; then
        cp -v src/pgbalancer "$BUILD_DIR/bin/"
        chmod +x "$BUILD_DIR/bin/pgbalancer"
    fi
    
    # Copy tools
    if [ -f "src/tools/pgmd5/pg_md5" ]; then
        cp -v src/tools/pgmd5/pg_md5 "$BUILD_DIR/bin/"
        chmod +x "$BUILD_DIR/bin/pg_md5"
    fi
    
    if [ -f "src/tools/pgenc/pg_enc" ]; then
        cp -v src/tools/pgenc/pg_enc "$BUILD_DIR/bin/"
        chmod +x "$BUILD_DIR/bin/pg_enc"
    fi
    
    if [ -f "src/tools/pgproto/pgproto" ]; then
        cp -v src/tools/pgproto/pgproto "$BUILD_DIR/bin/"
        chmod +x "$BUILD_DIR/bin/pgproto"
    fi
    
    if [ -f "src/tools/watchdog/wd_cli" ]; then
        cp -v src/tools/watchdog/wd_cli "$BUILD_DIR/bin/"
        chmod +x "$BUILD_DIR/bin/wd_cli"
    fi
    
    # Copy bctl if exists
    if [ -f "bctl/bctl" ]; then
        cp -v bctl/bctl "$BUILD_DIR/bin/"
        chmod +x "$BUILD_DIR/bin/bctl"
    fi
    
    # Copy setup scripts
    if [ -f "src/test/pgbalancer_setup" ]; then
        cp -v src/test/pgbalancer_setup "$BUILD_DIR/bin/"
        chmod +x "$BUILD_DIR/bin/pgbalancer_setup"
    fi
    
    if [ -f "src/test/watchdog_setup" ]; then
        cp -v src/test/watchdog_setup "$BUILD_DIR/bin/"
        chmod +x "$BUILD_DIR/bin/watchdog_setup"
    fi
    
    # Copy configuration files
    print_info "Copying configuration files to build/conf/"
    if [ -f "src/sample/pgpool.conf.sample" ]; then
        cp -v src/sample/pgpool.conf.sample "$BUILD_DIR/conf/pgbalancer.conf.sample"
    fi
    
    if [ -f "src/sample/pool_hba.conf.sample" ]; then
        cp -v src/sample/pool_hba.conf.sample "$BUILD_DIR/conf/"
    fi
    
    # Copy sample scripts
    if [ -d "src/sample/scripts" ]; then
        cp -rv src/sample/scripts "$BUILD_DIR/conf/"
    fi
    
    # Copy libraries
    print_info "Copying libraries to build/lib/"
    if [ -f "src/parser/libsql-parser.a" ]; then
        cp -v src/parser/libsql-parser.a "$BUILD_DIR/lib/"
    fi
    
    if [ -f "src/watchdog/lib-watchdog.a" ]; then
        cp -v src/watchdog/lib-watchdog.a "$BUILD_DIR/lib/"
    fi
    
    # Create README in build directory
    cat > "$BUILD_DIR/README.txt" << 'EOF'
pgbalancer v1.0.0 - Build Output
=================================

Directory Structure:
--------------------
bin/    - Executable binaries
conf/   - Configuration files and sample scripts
lib/    - Static libraries

Binaries:
---------
bin/pgbalancer          - Main pgbalancer server
bin/bctl                - REST API management utility
bin/pg_md5              - MD5 password utility
bin/pg_enc              - Encryption utility
bin/pgproto             - PostgreSQL protocol testing tool
bin/wd_cli              - Watchdog CLI utility
bin/pgbalancer_setup    - Setup utility
bin/watchdog_setup      - Watchdog setup utility

Configuration:
--------------
conf/pgbalancer.conf.sample - Main configuration file
conf/pool_hba.conf.sample   - Host-based authentication
conf/scripts/               - Sample operational scripts

Quick Start:
------------
1. Copy configuration files:
   cp conf/pgbalancer.conf.sample /etc/pgbalancer.conf
   cp conf/pool_hba.conf.sample /etc/pool_hba.conf

2. Edit configuration:
   vi /etc/pgbalancer.conf

3. Start pgbalancer:
   bin/pgbalancer -f /etc/pgbalancer.conf

4. Manage with bctl:
   bin/bctl --help

For more information, visit: https://pgelephant.github.io/pgbalancer/

Copyright (c) 2003-2021 PgPool Global Development Group
Copyright (c) 2025 pgElephant
EOF
    
    # Create version file
    echo "1.0.0" > "$BUILD_DIR/VERSION"
    
    # Create directory listing
    print_success "Build organized in: $BUILD_DIR"
    echo ""
    print_info "Directory structure:"
    tree -L 2 "$BUILD_DIR" 2>/dev/null || find "$BUILD_DIR" -type f -o -type d | head -30
    
    echo ""
    print_info "Build contents:"
    echo "  Binaries: $(find $BUILD_DIR/bin -type f 2>/dev/null | wc -l) files"
    echo "  Config files: $(find $BUILD_DIR/conf -type f 2>/dev/null | wc -l) files"
    echo "  Libraries: $(find $BUILD_DIR/lib -type f 2>/dev/null | wc -l) files"
}

install_build() {
    print_header "Installing pgbalancer"
    
    print_info "Installing to: $PREFIX"
    
    if [ -w "$PREFIX" ]; then
        make install
    else
        print_info "Requires sudo for installation"
        sudo make install
    fi
    
    print_success "Installation completed"
    print_info "pgbalancer installed to: $PREFIX/bin/pgbalancer"
}

#------------------------------------------------------------------------------
# Main Build Process
#------------------------------------------------------------------------------

print_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Build script for pgbalancer - PostgreSQL Connection Pooler

OPTIONS:
    -h, --help              Show this help message
    -c, --clean             Clean build (remove all generated files)
    -v, --verbose           Verbose output
    -j, --jobs N            Number of parallel jobs (default: $JOBS)
    -p, --prefix PATH       Installation prefix (default: $PREFIX)
    -s, --skip-deps         Skip dependency installation
    -i, --install           Install after building
    -t, --type TYPE         Build type: release|debug (default: $BUILD_TYPE)
    --pg-version VER        PostgreSQL version (default: $POSTGRESQL_VERSION)

ENVIRONMENT VARIABLES:
    BUILD_TYPE              Build type (release|debug)
    JOBS                    Number of parallel jobs
    PREFIX                  Installation prefix
    CLEAN_BUILD             Clean build (yes|no)
    SKIP_DEPS               Skip dependencies (yes|no)
    VERBOSE                 Verbose output (yes|no)
    POSTGRESQL_VERSION      PostgreSQL major version

EXAMPLES:
    # Basic build
    ./build.sh

    # Clean build with installation
    ./build.sh --clean --install

    # Debug build with verbose output
    ./build.sh --type debug --verbose

    # Custom prefix and PostgreSQL version
    ./build.sh --prefix /opt/pgbalancer --pg-version 16

EOF
}

main() {
    local do_install=no
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                print_usage
                exit 0
                ;;
            -c|--clean)
                CLEAN_BUILD=yes
                shift
                ;;
            -v|--verbose)
                VERBOSE=yes
                shift
                ;;
            -j|--jobs)
                JOBS="$2"
                shift 2
                ;;
            -p|--prefix)
                PREFIX="$2"
                shift 2
                ;;
            -s|--skip-deps)
                SKIP_DEPS=yes
                shift
                ;;
            -i|--install)
                do_install=yes
                shift
                ;;
            -t|--type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            --pg-version)
                POSTGRESQL_VERSION="$2"
                shift 2
                ;;
            *)
                print_error "Unknown option: $1"
                print_usage
                exit 1
                ;;
        esac
    done
    
    # Start build process
    print_header "pgbalancer Build System"
    print_info "Build type: $BUILD_TYPE"
    print_info "Jobs: $JOBS"
    print_info "Prefix: $PREFIX"
    print_info "PostgreSQL version: $POSTGRESQL_VERSION"
    echo ""
    
    # Detect OS
    detect_os
    echo ""
    
    # Install dependencies
    install_dependencies
    echo ""
    
    # Detect PostgreSQL
    detect_postgresql
    echo ""
    
    # Clean if requested
    if [ "$CLEAN_BUILD" = "yes" ]; then
        clean_build
        echo ""
    fi
    
    # Generate configure script
    if [ ! -f configure ]; then
        generate_configure
        echo ""
    fi
    
    # Configure
    configure_build
    echo ""
    
    # Compile
    compile_build
    echo ""
    
    # Verify
    verify_build
    echo ""
    
    # Organize build output
    organize_build
    echo ""
    
    # Install if requested
    if [ "$do_install" = "yes" ]; then
        install_build
        echo ""
    fi
    
    # Final summary
    print_header "Build Summary"
    print_success "pgbalancer built successfully!"
    print_info "Source binary: $SCRIPT_DIR/src/pgbalancer"
    print_info "Build output: $SCRIPT_DIR/build/"
    if [ "$do_install" = "yes" ]; then
        print_info "Installed: $PREFIX/bin/pgbalancer"
    else
        print_info "To install, run: ./build.sh --install"
        print_info "Or: sudo make install"
    fi
    echo ""
    print_info "To run pgbalancer:"
    print_info "  $SCRIPT_DIR/build/bin/pgbalancer --help"
    print_info "  Or: $SCRIPT_DIR/src/pgbalancer --help"
    echo ""
}

# Run main function
main "$@"

