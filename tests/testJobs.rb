# -*- coding: utf-8 -*-
require "test/unit"
require "expect"
require "pty"

require "../tests/testConstantes"

class Test3Jobs < Test::Unit::TestCase
  test_order=:defined

  def setup
    @pty_read, @pty_write, @pty_pid = PTY.spawn(COMMANDESHELL)
  end

  def teardown
    system("rm -f totoExpect.txt titiExpect.txt")
  end

  def test_inout
    @pty_write.puts("touch totoExpect.txt")
    @pty_write.puts("sleep 10 &")
    @pty_write.puts("ls -s totoExpect.txt")
    @pty_write.puts("jobs")
    a = @pty_read.expect(/0 totoExpect.txt/, DELAI)
    assert_not_nil(a, "le ls n'a pas afficher la taille du fichier")
    a = @pty_read.expect(/sleep/, DELAI)
    assert_not_nil(a, "jobs n'affiche pas le nom de la commande sleep")
  end
end
