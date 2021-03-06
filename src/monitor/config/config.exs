# This file is responsible for configuring your application
# and its dependencies with the aid of the Mix.Config module.
#
# This configuration file is loaded before any dependency and
# is restricted to this project.
use Mix.Config

# General application configuration
config :monitor,
  ecto_repos: [Monitor.Repo]

# Configures the endpoint
config :monitor, MonitorWeb.Endpoint,
  url: [host: "localhost"],
  secret_key_base: "CIKOpaJarfbz1Cdf83eqvOjSHztERYQVUtHWt0J5j5Rn5obWwGc3AQesIzzm07Ob",
  render_errors: [view: MonitorWeb.ErrorView, accepts: ~w(html json)],
  pubsub: [name: Monitor.PubSub,
           adapter: Phoenix.PubSub.PG2]

# Configures Elixir's Logger
config :logger, :console,
  format: "$time $metadata[$level] $message\n",
  metadata: [:user_id]

# Import environment specific config. This must remain at the bottom
# of this file so it overrides the configuration defined above.
import_config "#{Mix.env}.exs"
