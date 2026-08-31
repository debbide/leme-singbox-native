# leme-singbox-native

Leme Hub 使用的 sing-box 动态库、稳定 C ABI 和后续 Node-API 对接层。

目标平台：

- Windows x64
- Linux x64
- Linux ARM64

详细计划见 [docs/IMPLEMENTATION_PLAN.md](docs/IMPLEMENTATION_PLAN.md)。EOF
cat > .gitignore <<'EOF'
/dist/
/upstream/
/build/
*.dll
*.so
*.a
*.h
*.zip
*.tar.gz
