#pragma once

#include "borealis.hpp"

#include "dismiss_dialog.hpp"
#include "progress_bar.hpp"

#include <string>
#include <atomic>


class LoadingDialog : public brls::DismissDialog
{
  public:
    static LoadingDialog* build();

    void open() override;

    void setAction(const std::string& action);

    void setProgress(float progress);
    float getProgress();
    std::atomic<float>& getAtomicProgress();

  private:
    LoadingDialog(Box* contentView);
    ~LoadingDialog();

    std::string action;
    std::atomic<float> progress{0};

    brls::Label* label{nullptr};
    brls::Label* progressLabel{nullptr};
    ProgressBar* progressBar{nullptr};
};
