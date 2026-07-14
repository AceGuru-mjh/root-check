// APEX-Root Offline Root Key Generator
// Build: g++ -o generate_root_keys generate_root_keys.cpp -loqs -std=c++17
// Run on an air-gapped machine. NEVER expose the private key to the internet.

#include <oqs/oqs.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
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

int main() {
    printf("========================================\n");
    printf("APEX-Root 后量子根密钥生成工具 v1.0\n");
    printf("算法: %s\n", OQS_SIG_alg_dilithium_3);
    printf("========================================\n\n");

    OQS_SIG* sig = OQS_SIG_new(OQS_SIG_alg_dilithium_3);
    if (!sig) {
        fprintf(stderr, "错误: 无法初始化 %s\n", OQS_SIG_alg_dilithium_3);
        return 1;
    }

    printf("公钥长度: %zu 字节\n", sig->length_public_key);
    printf("私钥长度: %zu 字节\n", sig->length_secret_key);
    printf("签名长度: %zu 字节\n", sig->length_signature);
    printf("\n");

    // Seed with additional entropy
    std::srand((unsigned)std::time(nullptr) ^ (unsigned)clock());
    uint8_t seed[32];
    for (int i = 0; i < 32; i++)
        seed[i] = (uint8_t)(std::rand() & 0xFF);

    std::vector<uint8_t> pub(sig->length_public_key);
    std::vector<uint8_t> priv(sig->length_secret_key);

    OQS_STATUS status = OQS_SIG_keypair(sig, pub.data(), priv.data());
    if (status != OQS_SUCCESS) {
        fprintf(stderr, "错误: 密钥生成失败 (%d)\n", status);
        OQS_SIG_free(sig);
        return 1;
    }

    std::string pub_b64 = base64_encode(pub);
    std::string priv_b64 = base64_encode(priv);

    printf("✅ 密钥生成成功!\n\n");

    printf("--- 根公钥（复制到 APEX-Root 代码中） ---\n");
    printf("static const char* ROOT_PUBLIC_KEY_BASE64 =\n");
    // Split into 64-char lines for readability
    for (size_t i = 0; i < pub_b64.size(); i += 64) {
        printf("    \"%.*s\"\n", (int)((pub_b64.size() - i < 64) ? (pub_b64.size() - i) : 64), pub_b64.data() + i);
    }
    printf(";\n\n");

    printf("--- 根私钥（保存到离线安全介质，永不泄露） ---\n");
    for (size_t i = 0; i < priv_b64.size(); i += 64) {
        printf("%.*s\n", (int)((priv_b64.size() - i < 64) ? (priv_b64.size() - i) : 64), priv_b64.data() + i);
    }
    printf("\n");

    // Save to files
    FILE* fpub = fopen("apex_root_public.key", "w");
    if (fpub) { fprintf(fpub, "%s\n", pub_b64.c_str()); fclose(fpub); }

    FILE* fpriv = fopen("apex_root_private.key", "w");
    if (fpriv) { fprintf(fpriv, "%s\n", priv_b64.c_str()); fclose(fpriv); }

    printf("✅ 密钥已保存:\n");
    printf("  - apex_root_public.key\n");
    printf("  - apex_root_private.key\n\n");

    printf("⚠️  重要安全提示:\n");
    printf("1. 立即将 apex_root_private.key 移动到离线安全介质\n");
    printf("2. 删除本机上的所有私钥副本\n");
    printf("3. 根私钥仅用于签名官方规则包\n");
    printf("4. 永远不要将根私钥连接到互联网\n");

    OQS_SIG_free(sig);
    return 0;
}
