#include "subsonicsettingspage.h"
#include "ui_subsonicsettingspage.h"
#include "core/logging.h"
#include "internetmodel.h"

#include <QSettings>

SubsonicSettingsPage::SubsonicSettingsPage(SettingsDialog *dialog)
  : SettingsPage(dialog),
    ui_(new Ui_SubsonicSettingsPage),
    service_(InternetModel::Service<SubsonicService>())
{
  ui_->setupUi(this);
  setWindowIcon(QIcon(":/providers/subsonic-32.png"));

  connect(ui_->server, SIGNAL(editingFinished()),
          SLOT(ServerEditingFinished()));
  connect(ui_->login, SIGNAL(clicked()), SLOT(Login()));
  connect(ui_->login_state, SIGNAL(LogoutClicked()), SLOT(Logout()));
  connect(service_, SIGNAL(LoginStateChanged(SubsonicService::LoginState)),
          SLOT(LoginStateChanged(SubsonicService::LoginState)));

  ui_->login_state->AddCredentialField(ui_->server);
  ui_->login_state->AddCredentialField(ui_->username);
  ui_->login_state->AddCredentialField(ui_->password);
  ui_->login_state->AddCredentialGroup(ui_->server_group);

  ui_->login_state->SetAccountTypeText(tr(
      "Streaming from a Subsonic server requires a valid server license after the 30-day trial period."));
  ui_->login_state->SetAccountTypeVisible(true);
}

SubsonicSettingsPage::~SubsonicSettingsPage()
{
    delete ui_;
}

void SubsonicSettingsPage::Load()
{
  QSettings s;
  s.beginGroup(SubsonicService::kSettingsGroup);

  ui_->server->setText(s.value("server").toString());
  ui_->username->setText(s.value("username").toString());
  ui_->password->setText(s.value("password").toString());

  // If the settings are complete, SubsonicService will have used them already and
  // we can tell the user if they worked
  if (ui_->server->text() != "" && ui_->username->text() != "") {
    LoginStateChanged(service_->login_state());
  }
}

void SubsonicSettingsPage::Save()
{
  QSettings s;
  s.beginGroup(SubsonicService::kSettingsGroup);

  s.setValue("server", ui_->server->text());
  s.setValue("username", ui_->username->text());
  s.setValue("password", ui_->password->text());
}

void SubsonicSettingsPage::LoginStateChanged(SubsonicService::LoginState newstate)
{
  const bool logged_in = newstate == SubsonicService::LoginState_Loggedin;

  ui_->login_state->SetLoggedIn(logged_in ? LoginStateWidget::LoggedIn
                                          : LoginStateWidget::LoggedOut,
                                QString("%1 (%2)")
                                .arg(ui_->username->text())
                                .arg(ui_->server->text()));
  ui_->login_state->SetAccountTypeVisible(!logged_in);

  switch (newstate)
  {
  case SubsonicService::LoginState_BadServer:
    ui_->login_state->SetAccountTypeText(tr("Could not connect to Subsonic, check server URL. "
                                            "Example: http://localhost:4040/"));
    break;

  case SubsonicService::LoginState_BadCredentials:
    ui_->login_state->SetAccountTypeText(tr("Wrong username or password."));
    break;

  case SubsonicService::LoginState_OutdatedClient:
    ui_->login_state->SetAccountTypeText(tr("Incompatible Subsonic REST protocol version. Client must upgrade."));
    break;

  case SubsonicService::LoginState_OutdatedServer:
    ui_->login_state->SetAccountTypeText(tr("Incompatible Subsonic REST protocol version. Server must upgrade."));
    break;

  case SubsonicService::LoginState_Unlicensed:
    ui_->login_state->SetAccountTypeText(tr("The trial period for the Subsonic server is over. "
                                            "Please donate to get a license key. Visit subsonic.org for details."));
    break;

  case SubsonicService::LoginState_OtherError:
    ui_->login_state->SetAccountTypeText(tr("An unspecified error occurred."));
    break;

  default:
    break;
  }
}

void SubsonicSettingsPage::ServerEditingFinished() {
  QString input = ui_->server->text();
  QUrl url = QUrl::fromUserInput(input);

  // Veto things that don't get guessed as an HTTP URL, the result will be unhelpful
  if (!url.scheme().startsWith("http")) {
    return;
  }

  // A direct paste of the server URL will probably include the trailing index.view, so remove it
  if (url.path().endsWith("index.view")) {
    QString newpath = url.path();
    newpath.chop(10);
    url.setPath(newpath);
  }

  ui_->server->setText(url.toString());
  qLog(Debug) << "URL fixed:" << input << "to" << url;
}

void SubsonicSettingsPage::Login()
{
  ui_->login_state->SetLoggedIn(LoginStateWidget::LoginInProgress);
  service_->Login(ui_->server->text(), ui_->username->text(), ui_->password->text());
}

void SubsonicSettingsPage::Logout()
{
  ui_->username->setText("");
  ui_->password->setText("");
}
