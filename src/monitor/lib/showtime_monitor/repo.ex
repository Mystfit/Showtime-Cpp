defmodule ShowtimeMonitor.Repo do
  use Ecto.Repo,
    otp_app: :showtime_monitor,
    adapter: Ecto.Adapters.Postgres
end
