defmodule ShowtimeMonitorWeb.PageController do
  use ShowtimeMonitorWeb, :controller

  def index(conn, _params) do
    render(conn, "index.html")
  end
end
