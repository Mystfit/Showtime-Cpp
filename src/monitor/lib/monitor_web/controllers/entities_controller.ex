defmodule MonitorWeb.EntityController do
  use MonitorWeb, :controller

  def index(conn, _params) do
    #render conn, "index.html"
  end

  def edit(conn, _params) do
    IO.puts "In edit"
    #json conn, %{id: id}
  end

  def new(conn, _params) do
    IO.puts "In new"
    #json conn, %{id: id}
  end

  def show(conn, _params) do
    IO.puts "In show"
    #json conn, %{id: id}
  end

  def create(conn, _params) do
    IO.puts "In create"
    #json conn, %{id: id}
  end

  def update(conn, _params) do
    IO.puts "In update"
    #json conn, %{id: id}
  end

  def delete(conn, _params) do
   IO.puts "In delete"
   #json conn, %{id: id}
  end
end
