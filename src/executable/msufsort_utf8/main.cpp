
#include <library/msufsort_utf8.h>

#include <fmt/format.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <span>
#include <thread>

#include <concepts>



namespace
{

    //=========================================================================
    std::vector<char> load_file
    (
        std::string const & path
    )
    {
        std::vector<char> destination;
        std::ifstream source(path, std::ios_base::in | std::ios_base::binary);
        if (!source)
            throw std::runtime_error(fmt::format("Failed to load file: {}", path));
        source.seekg(0, std::ios_base::end);
        destination.resize(source.tellg());
        source.seekg(0, std::ios_base::beg);
        source.read(destination.data(), destination.size());
        source.close();
        return destination;
    }

} // namespace


//=============================================================================
int main
(
    int argc,
    char ** argv
)
{
    try
    {
        using namespace maniscalco;
        auto bwt = make_burrows_wheeler_transform(load_file(argv[1]));

        std::ofstream f(argv[2], std::ios_base::out | std::ios_base::binary);
        f.write((char const *)bwt.data(), bwt.size());
        f.close();

    }
    catch (std::exception const & exception)
    {
        std::cout << exception.what() << '\n';
    }


    return 0;
}
