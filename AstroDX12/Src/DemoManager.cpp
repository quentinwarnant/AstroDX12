#include <DemoManager.h>
#include <Rendering/Common/GPUPass.h>
#include <Common.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

void DemoManager::RegisterDemo(const std::string& name,
	std::vector<GPUPass*> passes,
	std::vector<std::string> dependencies)
{
	DemoDefinition demo;
	demo.name = name;
	demo.passes = std::move(passes);
	demo.dependencies = std::move(dependencies);
	demo.enabled = false;

	m_demoLookup[name] = m_demos.size();
	m_demos.push_back(std::move(demo));
}

void DemoManager::SetDemoEnabled(const std::string& name, bool enabled)
{
	auto it = m_demoLookup.find(name);
	if (it == m_demoLookup.end())
	{
		DX::astro_assert(false, "Demo not found");
		return;
	}

	m_demos[it->second].enabled = enabled;

	if (enabled)
	{
		EnableDependencies(name);
	}

	ApplyEnabledStates();
}

bool DemoManager::IsDemoEnabled(const std::string& name) const
{
	auto it = m_demoLookup.find(name);
	if (it == m_demoLookup.end())
		return false;

	return m_demos[it->second].enabled;
}

void DemoManager::EnableDependencies(const std::string& name)
{
	auto it = m_demoLookup.find(name);
	if (it == m_demoLookup.end())
		return;

	const auto& demo = m_demos[it->second];
	for (const auto& dep : demo.dependencies)
	{
		auto depIt = m_demoLookup.find(dep);
		if (depIt != m_demoLookup.end() && !m_demos[depIt->second].enabled)
		{
			m_demos[depIt->second].enabled = true;
			EnableDependencies(dep);
		}
	}
}

void DemoManager::ApplyEnabledStates()
{
	// First disable all passes
	for (auto& demo : m_demos)
	{
		for (auto* pass : demo.passes)
		{
			pass->SetEnabled(false);
		}
	}

	// Then enable passes for enabled demos
	for (auto& demo : m_demos)
	{
		if (demo.enabled)
		{
			for (auto* pass : demo.passes)
			{
				pass->SetEnabled(true);
			}
		}
	}
}

void DemoManager::LoadConfig(const std::string& configPath)
{
	m_configPath = configPath;

	std::ifstream file(configPath);
	if (!file.is_open())
	{
		// No config file yet - write defaults
		SaveConfig();
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		// Skip comments and empty lines
		if (line.empty() || line[0] == '#')
			continue;

		auto eqPos = line.find('=');
		if (eqPos == std::string::npos)
			continue;

		std::string name = line.substr(0, eqPos);
		std::string value = line.substr(eqPos + 1);

		// Trim whitespace
		name.erase(0, name.find_first_not_of(" \t"));
		name.erase(name.find_last_not_of(" \t") + 1);
		value.erase(0, value.find_first_not_of(" \t"));
		value.erase(value.find_last_not_of(" \t") + 1);

		auto it = m_demoLookup.find(name);
		if (it != m_demoLookup.end())
		{
			bool enabled = (value == "true" || value == "1");
			m_demos[it->second].enabled = enabled;

			if (enabled)
			{
				EnableDependencies(name);
			}
		}
	}

	ApplyEnabledStates();
}

void DemoManager::SaveConfig() const
{
	if (m_configPath.empty())
		return;

	std::ofstream file(m_configPath);
	if (!file.is_open())
		return;

	file << "# Astro DX12 Demo Configuration\n";
	file << "# Set demos to true/false to enable/disable them on startup\n\n";

	for (const auto& demo : m_demos)
	{
		file << demo.name << " = " << (demo.enabled ? "true" : "false") << "\n";
	}
}
