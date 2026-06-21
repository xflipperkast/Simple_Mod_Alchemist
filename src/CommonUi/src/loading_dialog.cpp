#include "loading_dialog.hpp"

#include "util.hpp"

LoadingDialog* LoadingDialog::build() {
  brls::Box* contentView = new brls::Box(brls::Axis::COLUMN);
  contentView->setHeight(300.0f);
  contentView->setJustifyContent(brls::JustifyContent::SPACE_AROUND);
  contentView->setAlignItems(brls::AlignItems::CENTER);
  Util::padContent(contentView);
  return new LoadingDialog(contentView);
}

void LoadingDialog::open() {
  brls::Application::pushActivity(new brls::Activity(this));
}

void LoadingDialog::setAction(const std::string& action) {
  this->label->setText(action.find("wait") == std::string::npos ? action + " Please wait..." : action);
}

void LoadingDialog::setProgress(float progress) {
  this->progress.store(progress);
}

float LoadingDialog::getProgress() {
  return this->progress.load();
}

std::atomic<float>& LoadingDialog::getAtomicProgress() {
  return this->progress;
}

LoadingDialog::LoadingDialog(Box* contentView): brls::DismissDialog(contentView) {
  this->label = new brls::Label();
  this->label->setText("Please wait...");
  contentView->addView(this->label);

  this->progressLabel = new brls::Label();
  this->progressLabel->setText("0%");
  contentView->addView(this->progressLabel);

  this->progressBar = new ProgressBar();
  this->progressBar->setProgress(0);
  contentView->addView(this->progressBar);

  this->setFocusable(true);
  this->setHideHighlight(true);
  this->setCancelable(false);
}

LoadingDialog::~LoadingDialog() {
}
