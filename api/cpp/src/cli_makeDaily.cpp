#include <cxxopts.hpp>
#include <dailyboy/api/log.hpp>
#include <dailyboy/api/makeDaily.hpp>
#include <exception>
#include <iostream>
#include <string>

namespace {

int run_with_job_path(const std::string& job_yaml_path) {
  return dailyboy::api::makeDaily(job_yaml_path);
}

void print_help(const cxxopts::Options& options) {
  std::cout << options.help() << '\n';
  std::cout << "Example:\n  makeDaily examples/job.mvp.example.yaml\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  try {
    cxxopts::Options options(
        "makeDaily", "DailyBoy — generate VFX dailies from a YAML job.");

    options.add_options()("j,job", "Path to the job file (.yaml / .yml)",
                          cxxopts::value<std::string>())(
        "v,verbose", "DEBUG log level")("h,help", "Show this help");

    options.positional_help("<job.yaml>");
    options.parse_positional({"job"});

    const auto parsed = options.parse(argc, argv);

    dailyboy::api::init_logging("makeDaily");
    if (parsed.count("verbose") != 0U) {
      dailyboy::api::set_log_level(dailyboy::api::LogLevel::Debug);
    }

    if (parsed.count("help") != 0U) {
      print_help(options);
      return 0;
    }

    if (parsed.count("job") == 0U) {
      std::cerr << "Error: job YAML path is required.\n\n";
      print_help(options);
      return 1;
    }

    return run_with_job_path(parsed["job"].as<std::string>());
  } catch (const cxxopts::exceptions::exception& ex) {
    std::cerr << "Argument error: " << ex.what() << "\n\n";
    return 1;
  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "Error: unknown failure.\n";
    return 1;
  }
}
