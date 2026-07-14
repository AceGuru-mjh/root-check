// APEX-Root Rule Package Signing Tool
// Usage: sign_rule_package <rule_package_file> <private_key_file>
// Output: <rule_package_file>.sig (Base64-encoded Dilithium signature)
//
// Build: g++ -o sign_rule_package sign_rule_package.cpp -loqs -std=c++17

#include <oqs/oqs.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static const std::string B64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static std::string base64_encode(const std::vector<uint8_t>& data) {
    std::string ret;
    int i = 0;
    uint8_t a3[3], a4[4];
    for (auto byte : data) {
        a3[i++] = byte;
        if (i == 3) {
            a4[0] = (a3[0] & 0xfc) >> 2;
            a4[1] = ((a3[0] & 0x03) << 4) | ((a3[1] & 0xf0) >> 4);
            a4[2] = ((a3[1] & 0x0f) << 2) | ((a3[2] & 0xc0) >> 6);
            a4[3] = a3[2] & 0x3f;
            for (i = 0; i < 4; i++) ret += B64_CHARS[a4[i]];
            i = 0;
        }
    }
    if (i) {
        for (int j = i; j < 3; j++) a3[j] = '\0';
        a4[0] = (a3[0] & 0xfc) >> 2;
        a4[1] = ((a3[0] & 0x03) << 4) | ((a3[1] & 0xf0) >> 4);
        a4[2] = ((a3[1] & 0x0f) << 2) | ((a3[2] & 0xc0) >> 6);
        a4[3] = a3[2] & 0x3f;
        for (int j = 0; j < i + 1; j++) ret += B64_CHARS[a4[j]];
        while (i++ < 3) ret += '=';
    }
    return ret;
}

static std::vector<uint8_t> base64_decode(const std::string& encoded) {
    std::vector<uint8_t> ret;
    int i = 0, pos = 0;
    uint8_t a4[4], a3[3];
    while (pos < (int)encoded.size() && encoded[pos] != '=') {
        auto c = encoded[pos];
        if (!(isalnum(c) || c == '+' || c == '/')) { pos++; continue; }
        a4[i++] = (uint8_t)B64_CHARS.find(c);
        if (i == 4) {
            a3[0] = (a4[0] << 2) | ((a4[1] & 0x30) >> 4);
            a3[1] = ((a4[1] & 0x0f) << 4) | ((a4[2] & 0x3c) >> 2);
            a3[2] = ((a4[2] & 0x03) << 6) | a4[3];
            for (i = 0; i < 3; i++) ret.push_back(a3[i]);
            i = 0;
        }
        pos++;
    }
    if (i) {
        a3[0] = (a4[0] << 2) | ((a4[1] & 0x30) >> 4);
        a3[1] = ((a4[1] & 0x0f) << 4) | ((a4[2] & 0x3c) >> 2);
        for (int j = 0; j < i - 1; j++) ret.push_back(a3[j]);
    }
    return ret;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "用法: sign_rule_package <规则包文件> <私钥文件>\n");
        return 1;
    }

    const char* rule_file = argv[1];
    const char* key_file = argv[2];

    // Read rule package
    FILE* fr = fopen(rule_file, "rb");
    if (!fr) { fprintf(stderr, "错误: 无法打开 %s\n", rule_file); return 1; }
    fseek(fr, 0, SEEK_END);
    long rule_size = ftell(fr);
    fseek(fr, 0, SEEK_SET);
    std::vector<uint8_t> rule_data(rule_size);
    if (fread(rule_data.data(), 1, rule_size, fr) != (size_t)rule_size) {
        fprintf(stderr, "错误: 读取规则包失败\n");
        fclose(fr);
        return 1;
    }
    fclose(fr);

    // Read private key (Base64)
    FILE* fk = fopen(key_file, "r");
    if (!fk) { fprintf(stderr, "错误: 无法打开 %s\n", key_file); return 1; }
    char key_buf[65536];
    size_t key_len = fread(key_buf, 1, sizeof(key_buf) - 1, fk);
    fclose(fk);
    key_buf[key_len] = '\0';

    // Remove whitespace
    std::string key_str;
    for (size_t i = 0; i < key_len; i++) {
        if (key_buf[i] != '\n' && key_buf[i] != '\r' && key_buf[i] != ' ')
            key_str += key_buf[i];
    }

    std::vector<uint8_t> private_key = base64_decode(key_str);

    // Initialize liboqs
    OQS_SIG* sig = OQS_SIG_new(OQS_SIG_alg_dilithium_3);
    if (!sig) {
        fprintf(stderr, "错误: 无法初始化 %s\n", OQS_SIG_alg_dilithium_3);
        return 1;
    }

    if (private_key.size() != sig->length_secret_key) {
        fprintf(stderr, "错误: 私钥长度不匹配 (期望 %zu, 实际 %zu)\n",
            sig->length_secret_key, private_key.size());
        OQS_SIG_free(sig);
        return 1;
    }

    // Sign
    std::vector<uint8_t> signature(sig->length_signature);
    size_t sig_len;
    OQS_STATUS status = OQS_SIG_sign(sig, signature.data(), &sig_len,
        rule_data.data(), rule_data.size(), private_key.data());

    if (status != OQS_SUCCESS) {
        fprintf(stderr, "错误: 签名失败 (%d)\n", status);
        OQS_SIG_free(sig);
        return 1;
    }
    signature.resize(sig_len);

    // Save signature
    std::string sig_path = std::string(rule_file) + ".sig";
    FILE* fs = fopen(sig_path.c_str(), "w");
    if (!fs) {
        fprintf(stderr, "错误: 无法写入 %s\n", sig_path.c_str());
        OQS_SIG_free(sig);
        return 1;
    }

    std::string sig_b64 = base64_encode(signature);
    // Split into 64-char lines
    for (size_t i = 0; i < sig_b64.size(); i += 64) {
        fprintf(fs, "%.*s\n",
            (int)((sig_b64.size() - i < 64) ? (sig_b64.size() - i) : 64),
            sig_b64.data() + i);
    }
    fclose(fs);

    printf("✅ 规则包签名成功!\n");
    printf("规则包:  %s (%zu 字节)\n", rule_file, rule_data.size());
    printf("签名文件: %s (%zu 字节签名)\n", sig_path.c_str(), sig_len);
    printf("算法:    %s\n", OQS_SIG_alg_dilithium_3);

    OQS_SIG_free(sig);
    return 0;
}
