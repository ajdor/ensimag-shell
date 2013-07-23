#!/usr/bin/ruby
# -*- coding: utf-8 -*-

# Copyright (C) 2013 Gregory Mounie
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or (at
# your option) any later version.
 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

require "test/unit"
require "expect"
require "pty"

if not defined? PROMPT then
    PROMPT=/^ensishell>/
    DELAI=1
end

class TestForkExec < Test::Unit::TestCase

  def setup
    @pty_read, @pty_write, @pty_pid = PTY.spawn("./ensishell")
  end

  def teardown
    # ne rien faire
  end

  def test_seq
    @pty_write.puts("seq 0 3")
    a = @pty_read.expect(/0\r\n1\n\n2\r\n3\r\n/m, DELAI)
    assert_not_equal(nil, a, "Sortie incohérente pour 'seq 0 3'")
  end

end
