#include <exception>
#include <iostream>
#include <string>

void test_wire();
void test_ipc();
void test_host();
void test_compositor();
void test_i18n();
int run_ipc_child(const std::string& name);

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--ipc-child") return run_ipc_child(argv[2]);
    try {
        test_wire();
        test_ipc();
        test_host();
        test_compositor();
        test_i18n();
        std::cout << "ALL TEST SUITES PASSED\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
