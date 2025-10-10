# SEP-UG-33 Block Model Project

A C++ implementation of a 3D block model compression algorithm that reads model specifications and performs block fitting and growth operations for optimal space utilisation.

## Project Overview

This project implements a complete block-based 3D model compression system with the following workflow: input processing → block fitting algorithm → compressed output → validation. The system provides:

- **Advanced Compression Algorithm**: Block growth algorithm for optimal 3D space utilisation
- **Cross-Platform Support**: Native Linux builds with automated Windows cross-compilation
- **Comprehensive Testing**: Unit tests, integration tests, and output validation framework
- **Professional Code Quality**: C++17 standard, automated formatting, static analysis
- **Complete Documentation**: Technical guides, coding standards, and workflow documentation

## Project Structure

```
SEP-UG-33/
├── src/                    # Source files (main.cpp, block.cpp, block_growth.cpp, block_model.cpp)
├── include/               # Header files (.h)
├── tests/                 # Test programs and test data (case1.txt, case2.txt)
├── docs/                  # Complete documentation (PDFs)
├── scripts/              # Development automation scripts
├── build/                # Build output directory
└── Makefile              # Build system configuration
```

## Development Environment

### Automated Setup

Execute the following command for complete environment configuration:

```bash
make install-deps
```

This command installs the C++ toolchain, formatting tools, cross-compilation tools for Windows, and configures IDE settings. Compatible with Ubuntu/Debian, macOS, and Windows platforms.

### Manual Setup

For manual installation or troubleshooting, consult the [Development Environment Guide](docs/Dev%20Environment%20Guide.pdf) which provides:

- Step-by-step compiler and tool installation procedures
- IDE configuration (VSCode, CLion) with IntelliSense setup
- vcpkg package management configuration
- CMake build system setup

### Prerequisites

- g++ compiler with C++17 support
- make utility
- mingw-w64 (for Windows cross-compilation)

## Contributing

### Development Workflow

1. **Environment Setup**: Execute `make install-deps`
2. **Branch Creation**: `git checkout -b UG33-<number>-<description>`
3. **Code Development**: Implement changes following established coding standards
4. **Code Formatting**: Execute `./scripts/format-code.sh` (mandatory)
5. **Testing**: Execute `make test-all` (all tests must pass)
6. **Cross-Platform Verification**: Execute `make windows-package`
7. **Pull Request**: Submit for team review and approval

### Quality Requirements

All contributions must satisfy the following criteria:

- Code formatted using `./scripts/format-code.sh` (clang-format)
- All tests pass: `make test-all`
- Windows build successful: `make windows-package`
- Explicit `std::` prefixes (no `using namespace std`)
- Conventional commit message format (e.g., `feat:`, `fix:`, `docs:`)

Refer to the [Git & GitHub Workflow Guide](docs/Git%20%26%20Github%20Workflow.pdf) for comprehensive branching strategy, commit conventions, and pull request procedures.

## Documentation

The project includes comprehensive documentation covering all development aspects:

### Developer Resources

- **[Development Environment Guide](docs/Dev%20Environment%20Guide.pdf)**: Complete setup instructions for all platforms, IDE configuration, and troubleshooting
- **[Coding Standards & Quality Guidelines](docs/Coding%20Standards.pdf)**: Code style requirements, naming conventions, static analysis, and testing standards
- **[Git & GitHub Workflow Guide](docs/Git%20%26%20Github%20Workflow.pdf)**: Version control workflow, branching strategy, and collaboration process

### Technical Reference

- **[Project Structure & Build Guide](docs/Project%20Structure.pdf)**: Complete build system reference, testing framework, and cross-platform compilation
- **[Jira Workflow Guidelines](docs/Jira%20Workflow.pdf)**: Project management, ticket creation, and task tracking

Recommended reading sequence: Development Environment Guide → Coding Standards → Git Workflow Guide.

## Build & Test Instructions

### Essential Commands

```bash
# Primary build and test operations
make test-all             # Execute all tests (unit + integration)
make                      # Build main executable
make windows-package      # Generate Windows .exe.zip for deployment

# Development operations
make test-compression-unit # Execute algorithm unit tests
make test-integration     # Execute end-to-end pipeline tests
make clean                # Remove build artifacts
make help                 # Display all available targets
```

### Build System

The build system provides automated dependency management, cross-platform compilation, and comprehensive test execution. Execute `make help` for complete target descriptions and usage information.

For build troubleshooting, consult the [Project Structure & Build Guide](docs/Project%20Structure.pdf).

## Algorithm Overview

The compression algorithm processes 3D voxel data through a sophisticated block-fitting methodology:

1. **Input Processing**: Reads model specifications, tag tables, and voxel grid data
2. **Block Growth Algorithm**: Identifies optimal rectangular blocks to minimise compressed size
3. **Output Generation**: Produces compressed format (`x,y,z,width,height,depth,label`)
4. **Validation**: Reconstructs the original model to verify compression accuracy

Sample data files in `tests/data/` (case1.txt, case2.txt) demonstrate the input format and provide realistic test scenarios.

## Quick Links

- **Test Data**: `tests/data/` (case1.txt, case2.txt sample files)
- **Build Output**: `build/` (compiled executables and artifacts)
- **Development Scripts**: `scripts/` (formatting, environment setup)
- **Build Reference**: `make help` (complete target documentation)
- **Documentation**: `docs/` (comprehensive PDF guides)


