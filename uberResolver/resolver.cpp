#include "resolver.h"

#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/resolverContext.h"

#include <pxr/usd/ar/defineResolver.h>

#include <map>
#include <cstdio>
#include <mutex>
#include <thread>
#include <fstream>

namespace {
    constexpr const char* placeholderURI = "sphere://";
    const char* placeholderAsset = R"usd(#usda 1.0
(
    defaultPrim = "hello"
)

def Xform "hello"
{
    def Sphere "world"
    {
    }
})usd";

    // Temp solution
    std::mutex g_resolvedNamesMutex;
    using mutex_scoped_lock = std::lock_guard<std::mutex>;
    std::map<std::string, std::string> g_resolvedNames;

    std::string resolveName(const std::string path)
    {
        mutex_scoped_lock sc(g_resolvedNamesMutex);
        const auto it = g_resolvedNames.find(path);
        if (it != g_resolvedNames.end()) {
            return it->second;
        } else {
            const auto resolvedName = std::tmpnam(nullptr) + std::string(".usda");
            g_resolvedNames.insert(std::make_pair(path, resolvedName));
            return resolvedName;
        }
    }
}

AR_DEFINE_RESOLVER(uberResolver, ArResolver)

uberResolver::uberResolver() : ArDefaultResolver()
{
}

uberResolver::~uberResolver()
{
}

std::string uberResolver::Resolve(const std::string& path)
{
    return uberResolver::ResolveWithAssetInfo(path, nullptr);
}

std::string uberResolver::ResolveWithAssetInfo(
    const std::string& path,
    ArAssetInfo* assetInfo)
{
    if (path.find(placeholderURI) == 0) {
        const auto resolved_name = resolveName(path);
        return resolved_name;
    } else {
        return ArDefaultResolver::ResolveWithAssetInfo(path, assetInfo);
    }
}

void uberResolver::UpdateAssetInfo(
    const std::string& identifier,
    const std::string& filePath,
    const std::string& fileVersion,
    ArAssetInfo* assetInfo)
{
    ArDefaultResolver::UpdateAssetInfo(identifier, filePath, fileVersion, assetInfo);
}

VtValue uberResolver::GetModificationTimestamp(
    const std::string& path,
    const std::string& resolvedPath)
{
    if (path.find(placeholderURI) == 0) {
        return VtValue(1.0);
    } else {
        return ArDefaultResolver::GetModificationTimestamp(path, resolvedPath);
    }
}

bool uberResolver::FetchToLocalResolvedPath(const std::string& path, const std::string& resolvedPath)
{
    if (path.find(placeholderURI) == 0) {
        std::fstream fs(resolvedPath, std::ios::out);
        fs << placeholderAsset;
        return true;
    } else {
        return true;
    }
}
